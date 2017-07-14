#! /usr/bin/env python
"""
This script tests running and closing moos_comms.
"""

import unittest
import subprocess
import time
import logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('tests')
logger.level = logging.DEBUG


try:
    import pymoos
except ImportError as e:
    logger.error('Could not find pymoos package. Is it installed?')
    raise e

from  abstractmoostest import pyMOOSTestCase


class RunCloseTest(pyMOOSTestCase):
    def test_run_close(self):
        c = pymoos.comms()

        self.assertFalse(c.is_connected())
        self.assertFalse(c.is_running())

        c.run('localhost', 9000, 'test_run_close')

        self.assertTrue(c.is_running())

        # sleep for a little to make sure we have time to connect
        c.wait_until_connected(2000)

        self.assertTrue(c.is_connected())
        self.assertTrue(c.is_asynchronous())

        c.close(True)

        self.assertFalse(c.is_connected())
        self.assertFalse(c.is_running())

    def test_name(self):
        c = pymoos.comms()
        c.run('localhost', 9000, 'test_name')
        c.wait_until_connected(2000)

        self.assertEqual(c.get_moos_name(), 'test_name')
        self.assertEqual(c.get_community_name(), 'pymoos_test_db')

        c.close(True)


if __name__ == '__main__':
    unittest.main()
