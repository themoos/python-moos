#! /usr/bin/env python
"""
This script tests some of the base functionalities of python-moos.
"""

import unittest
import subprocess
import time
import logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('tests')
# logger.level = logging.DEBUG


try:
    from pymoos import pymoos
except ImportError as e:
    logger.error('Could not find pymoos package. Is it installed?')
    raise e

class pyMOOSTestCase(unittest.TestCase):
    def setUp(self):
        try:
            logger.info('starting MOOSDB')
            self.moosdb_process = subprocess.Popen(['MOOSDB'])
            logger.info('starting MOOSDB done')
        except OSError as ose:
            logger.error("Error while launching MOOSDB !"
                        "Is core-moos installed?\n")
            raise ose

    def tearDown(self):
        logger.info('killing MOOSDB')
        self.moosdb_process.kill()
        logger.info('killing MOOSDB done')

    def test_run_close(self):
        # self.assertFalse(True)
        comms = pymoos.comms()
        self.assertFalse(comms.is_connected())
        comms.run('localhost', 9000, 'pymoos_test')
        # sleep for a little to make sure we have time to connect
        time.sleep(2)
        self.assertTrue(comms.is_connected())
        comms.close(True)
        self.assertFalse(comms.is_connected())



if __name__ == '__main__':
    unittest.main()
