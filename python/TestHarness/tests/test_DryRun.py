import subprocess
from TestHarnessTestCase import TestHarnessTestCase

class TestHarnessTester(TestHarnessTestCase):
    def testDryRun(self):
        """
        Test that --dry-run returns a passing status
        """
        output = self.runTests('-i', 'diffs', '--dry-run')

        self.assertRegexpMatches(output, 'test_harness\.exodiff.*?DRY RUN')
        self.assertRegexpMatches(output, 'test_harness\.csvdiff.*?DRY RUN')

        # Skipped caveat test which returns skipped instead of 'DRY RUN'
        output = self.runTests('--no-color', '-i', 'depend_skip_tests', '--dry-run')
        self.assertRegexpMatches(output, r'tests/test_harness.always_skipped.*? \[ALWAYS SKIPPED\] SKIP')
        self.assertRegexpMatches(output, r'tests/test_harness.needs_always_skipped.*? \[SKIPPED DEPENDENCY\] SKIP')

        # Deleted caveat test which returns a deleted failing tests while
        # performing a dry run
        with self.assertRaises(subprocess.CalledProcessError) as cm:
            self.runTests('--no-color', '-i', 'deleted', '-e', '--dry-run')

        e = cm.exception
        self.assertRegexpMatches(e.output, r'test_harness\.deleted.*? \[TEST DELETED TEST\] FAILED \(DELETED\)')
