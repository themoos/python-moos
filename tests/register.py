#! /usr/bin/env python
"""
This script tests registering for MOOS variables.
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


class RegisterTest(pyMOOSTestCase):
    def test_register(self):
        # self.assertFalse(True)
        c = pymoos.comms()
        c.run('localhost', 9000, 'test_register')
        c.wait_until_connected(2000)

        self.assertFalse(c.is_registered_for('MOOS_TIME'))
        self.assertFalse(c.is_registered_for('MOOS_UPTIME'))

        c.register('MOOS_UPTIME',0.)

        self.assertFalse(c.is_registered_for('MOOS_TIME'))
        self.assertTrue(c.is_registered_for('MOOS_UPTIME'))

        c.close(True)

    def test_get_registered(self):
        c = pymoos.comms()
        c.run('localhost', 9000, 'test_get_registered')
        c.wait_until_connected(2000)

        c.register('MOOS_UPTIME',0.)
        c.register('MOOS_UPTIME2',0.)

        self.assertSetEqual(c.get_registered(),
                {'MOOS_UPTIME', 'MOOS_UPTIME2'})

        c.close(True)

    def test_is_registered_for(self):
        c = pymoos.comms()
        c.run('localhost', 9000, 'test_is_registered_for')
        c.wait_until_connected(2000)

        self.assertFalse(c.is_registered_for('MOOS_UPTIME'))

        c.register('MOOS_UPTIME', 0.)

        self.assertTrue(c.is_registered_for('MOOS_UPTIME'))
        self.assertFalse(c.is_registered_for('MOOS_UPTIME2'))

        c.close(True)


if __name__ == '__main__':
    unittest.main()
