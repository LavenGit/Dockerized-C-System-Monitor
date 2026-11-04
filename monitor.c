#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;
static void stop(int signum) { (void)signum; running = 0; }

typedef struct {
    unsigned long long total_kib, available_kib;
    unsigned long long limit_bytes, usage_bytes, cpu_usage_us;
    unsigned long long cpu_quota_us, cpu_period_us;
    bool has_total, has_available, has_limit, has_usage, has_cpu_usage, has_cpu_quota;
    long online_cpus;
    long pid, ppid, uid, gid;
    char hostname[256], os[128], kernel[256];
} Snapshot;

static bool read_line(const char *path, char *buf, size_t size) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    bool ok = fgets(buf, (int)size, f) != NULL;
    fclose(f);
    if (ok) buf[strcspn(buf, "\n")] = '\0';
    return ok;
}

static bool parse_ull(const char *s, unsigned long long *value) {
    if (!s || *s == '-' || *s == '\0') return false;
    errno = 0;
    char *end;
    unsigned long long n = strtoull(s, &end, 10);
    if (errno || end == s || (*end != '\0' && *end != ' ' && *end != '\t')) return false;
    *value = n;
    return true;
}

static void collect(Snapshot *s) {
    memset(s, 0, sizeof(*s));
    struct utsname u;
    if (uname(&u) == 0) {
        snprintf(s->os, sizeof(s->os), "%s", u.sysname);
        snprintf(s->kernel, sizeof(s->kernel), "%s", u.release);
    }
    if (gethostname(s->hostname, sizeof(s->hostname)) != 0)
        snprintf(s->hostname, sizeof(s->hostname), "unknown");
    s->hostname[sizeof(s->hostname) - 1] = '\0';
    s->pid = (long)getpid(); s->ppid = (long)getppid();
    s->uid = (long)getuid(); s->gid = (long)getgid();
    s->online_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            unsigned long long n;
            if (sscanf(line, "MemTotal: %llu kB", &n) == 1) {
                s->total_kib = n; s->has_total = true;
            } else if (sscanf(line, "MemAvailable: %llu kB", &n) == 1) {
                s->available_kib = n; s->has_available = true;
            }
        }
        fclose(f);
    }
    char line[256];
    if (read_line("/sys/fs/cgroup/memory.max", line, sizeof(line)) &&
        strcmp(line, "max") != 0)
        s->has_limit = parse_ull(line, &s->limit_bytes);
    if (read_line("/sys/fs/cgroup/memory.current", line, sizeof(line)))
        s->has_usage = parse_ull(line, &s->usage_bytes);
    if (read_line("/sys/fs/cgroup/cpu.max", line, sizeof(line))) {
        char quota[64], period[64];
        if (sscanf(line, "%63s %63s", quota, period) == 2 &&
            strcmp(quota, "max") != 0 &&
            parse_ull(quota, &s->cpu_quota_us) &&
            parse_ull(period, &s->cpu_period_us) && s->cpu_period_us != 0)
            s->has_cpu_quota = true;
    }
    f = fopen("/sys/fs/cgroup/cpu.stat", "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "usage_usec ", 11) == 0) {
                s->has_cpu_usage = parse_ull(line + 11, &s->cpu_usage_us);
                break;
            }
        }
        fclose(f);
    }
}

static double elapsed_seconds(const struct timespec *a, const struct timespec *b) {
    return (double)(b->tv_sec - a->tv_sec) +
           (double)(b->tv_nsec - a->tv_nsec) / 1000000000.0;
}

/* CPU percent is relative to the cgroup quota, if present; otherwise one CPU. */
static bool cpu_percent(const Snapshot *previous, const Snapshot *current,
                        double elapsed, double *out) {
    if (!previous || !previous->has_cpu_usage || !current->has_cpu_usage ||
        current->cpu_usage_us < previous->cpu_usage_us || elapsed <= 0) return false;
    double cores = current->has_cpu_quota
        ? (double)current->cpu_quota_us / (double)current->cpu_period_us : 1.0;
    if (cores <= 0) return false;
    *out = (double)(current->cpu_usage_us - previous->cpu_usage_us) /
           (elapsed * 1000000.0 * cores) * 100.0;
    return true;
}

static void json_string(const char *s) {
    putchar('"');
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { putchar('\\'); putchar(c); }
        else if (c < 32) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

static void print_json(const Snapshot *s, bool has_cpu_pct, double cpu_pct) {
    printf("{\"hostname\":"); json_string(s->hostname);
    printf(",\"pid\":%ld,\"ppid\":%ld,\"uid\":%ld,\"gid\":%ld",
           s->pid, s->ppid, s->uid, s->gid);
    printf(",\"os\":"); json_string(s->os);
    printf(",\"kernel\":"); json_string(s->kernel);
    printf(",\"online_cpus\":%ld", s->online_cpus);
    printf(",\"host_memory_total_mib\":");
    if (s->has_total) printf("%.2f", s->total_kib / 1024.0); else printf("null");
    printf(",\"host_memory_available_mib\":");
    if (s->has_available) printf("%.2f", s->available_kib / 1024.0); else printf("null");
    printf(",\"memory_limit_mib\":");
    if (s->has_limit) printf("%.2f", s->limit_bytes / 1048576.0); else printf("null");
    printf(",\"memory_usage_mib\":");
    if (s->has_usage) printf("%.2f", s->usage_bytes / 1048576.0); else printf("null");
    printf(",\"memory_percent\":");
    if (s->has_limit && s->has_usage && s->limit_bytes > 0)
        printf("%.2f", 100.0 * (double)s->usage_bytes / (double)s->limit_bytes);
    else printf("null");
    printf(",\"cpu_quota_cores\":");
    if (s->has_cpu_quota) printf("%.3f", (double)s->cpu_quota_us / s->cpu_period_us);
    else printf("null");
    printf(",\"cpu_percent\":");
    if (has_cpu_pct) printf("%.2f", cpu_pct); else printf("null");
    puts("}");
    fflush(stdout);
}

static void print_text(const Snapshot *s, bool has_cpu_pct, double cpu_pct) {
    puts("========== C SYSTEM MONITOR ==========");
    printf("Hostname: %s\nPID: %ld | Parent PID: %ld\nUID: %ld | GID: %ld\n",
           s->hostname, s->pid, s->ppid, s->uid, s->gid);
    printf("OS: %s | Kernel: %s\nOnline CPUs (host-visible): %ld\n",
           s->os, s->kernel, s->online_cpus);
    if (s->has_total) printf("Host-visible memory total: %.2f MiB\n", s->total_kib / 1024.0);
    if (s->has_available) printf("Host-visible memory available: %.2f MiB\n", s->available_kib / 1024.0);
    if (s->has_limit) printf("Cgroup memory limit: %.2f MiB\n", s->limit_bytes / 1048576.0);
    else puts("Cgroup memory limit: unlimited or unavailable");
    if (s->has_usage) printf("Cgroup memory usage: %.2f MiB\n", s->usage_bytes / 1048576.0);
    if (s->has_limit && s->has_usage && s->limit_bytes > 0)
        printf("Cgroup memory utilization: %.2f%%\n", 100.0 * (double)s->usage_bytes / s->limit_bytes);
    if (s->has_cpu_quota)
        printf("Cgroup CPU quota: %.3f cores\n", (double)s->cpu_quota_us / s->cpu_period_us);
    else puts("Cgroup CPU quota: unlimited or unavailable");
    if (has_cpu_pct) printf("Cgroup CPU utilization (quota-relative): %.2f%%\n", cpu_pct);
    else puts("Cgroup CPU utilization: requires two samples (--watch)");
    puts("======================================");
    fflush(stdout);
}

static void usage(const char *name) {
    fprintf(stderr, "Usage: %s [--json] [--watch [seconds]]\n", name);
}

int main(int argc, char **argv) {
    bool json = false, watch = false;
    unsigned int interval = 2;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0) json = true;
        else if (strcmp(argv[i], "--watch") == 0) {
            watch = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                unsigned long long n;
                if (!parse_ull(argv[++i], &n) || n < 1 || n > 3600) {
                    usage(argv[0]); return 2;
                }
                interval = (unsigned int)n;
            }
        } else { usage(argv[0]); return 2; }
    }
    struct sigaction sa = {0};
    sa.sa_handler = stop;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    Snapshot previous = {0}, current;
    struct timespec before = {0}, now;
    bool have_previous = false;
    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
        collect(&current);
        double pct = 0;
        bool has_pct = have_previous &&
            cpu_percent(&previous, &current, elapsed_seconds(&before, &now), &pct);
        if (!json && watch) printf("\033[2J\033[H");
        if (json) print_json(&current, has_pct, pct);
        else print_text(&current, has_pct, pct);
        if (!watch || !running) break;
        previous = current; before = now; have_previous = true;
        struct timespec remaining = {(time_t)interval, 0};
        while (running && nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {}
    } while (running);
    return 0;
}
