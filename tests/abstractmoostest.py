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
    import pymoos
except ImportError as e:
    logger.error('Could not find pymoos package. Is it installed?')
    raise e

class pyMOOSTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        try:
            logger.info('starting MOOSDB ...')
            cls.moosdb_process = subprocess.Popen(['MOOSDB',
                            '--moos_community=pymoos_test_db'])
            logger.info('starting MOOSDB done')
        except OSError as ose:
            logger.error("Error while launching MOOSDB !"
                        "Is core-moos installed?\n")
            raise ose

    @classmethod
    def tearDownClass(cls):
        logger.info('killing MOOSDB ...')
        cls.moosdb_process.kill()
        logger.info('killing MOOSDB done')


if __name__ == '__main__':
    unittest.main()
