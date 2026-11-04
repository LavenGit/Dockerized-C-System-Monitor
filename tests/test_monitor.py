import json
import subprocess
import unittest


class MonitorTests(unittest.TestCase):
    def test_json_snapshot(self):
        result = subprocess.run(['./monitor', '--json'], capture_output=True,
                                text=True, check=True, timeout=10)
        data = json.loads(result.stdout)
        for key in ('hostname', 'pid', 'uid', 'gid', 'online_cpus',
                    'memory_limit_mib', 'memory_usage_mib', 'cpu_quota_cores',
                    'cpu_percent'):
            self.assertIn(key, data)
        self.assertIsNone(data['cpu_percent'])  # Needs two readings.

    def test_text_snapshot(self):
        result = subprocess.run(['./monitor'], capture_output=True,
                                text=True, check=True, timeout=10)
        self.assertIn('C SYSTEM MONITOR', result.stdout)
        self.assertIn('UID:', result.stdout)

    def test_invalid_interval(self):
        result = subprocess.run(['./monitor', '--watch', '0'],
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 2)

    def test_watch_json_stream(self):
        process = subprocess.Popen(['./monitor', '--json', '--watch', '1'],
                                   stdout=subprocess.PIPE, text=True)
        try:
            first = json.loads(process.stdout.readline())
            second = json.loads(process.stdout.readline())
            self.assertEqual(first['hostname'], second['hostname'])
            if second['cpu_percent'] is not None:
                self.assertGreaterEqual(second['cpu_percent'], 0)
        finally:
            process.terminate()
            process.communicate(timeout=5)


if __name__ == '__main__':
    unittest.main()
