#! /usr/bin/env python
"""
This script tests simple moos_comms.
"""

import unittest
import subprocess
import time
import logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('tests')
logger.level = logging.DEBUG


try:
    from pymoos import pymoos
except ImportError as e:
    logger.error('Could not find pymoos package. Is it installed?')
    raise e

from  abstractmoostest import pyMOOSTestCase


class RunCloseTest(pyMOOSTestCase):
    def test_notify_msg(self):
        c = pymoos.comms()
        c.set_on_connect_callback(
                lambda : self.assertTrue(c.register('SIMPLE_VAR', 0)))
        c.run('localhost', 9000, 'test_notify_msg')
        c.wait_until_connected(2000)

        self.assertTrue(c.is_registered_for('SIMPLE_VAR'))

        t = pymoos.time()

        self.assertTrue(c.notify('SIMPLE_VAR', 1., t))

        time.sleep(1)

        msgs = c.fetch()
        self.assertNotEqual(len(msgs), 0)
        for msg in msgs:
            self.assertEqual(msg.key(), 'SIMPLE_VAR')
            self.assertEqual(msg.name(), 'SIMPLE_VAR')
            self.assertTrue(msg.is_name('SIMPLE_VAR'))
            self.assertEqual(msg.double(), 1)
            self.assertTrue(msg.is_double())
            self.assertEqual(msg.source(), 'test_notify_msg')
            self.assertFalse(msg.is_string())
            self.assertFalse(msg.is_binary())
            self.assertEqual(msg.time(), t)

        c.close(True)

    def test_zmsg_types(self):
        x = bytearray( [0, 3, 0x15, 2, 6, 0xAA] )
        s = 'hello'
        d = 384.653
        c = pymoos.comms()

        def on_connect():
            self.assertTrue(c.register('STRING_VAR', 0))
            self.assertTrue(c.register('DOUBLE_VAR', 0))
            self.assertTrue(c.register('BINARY_VAR', 0))
            return True
        #
        # def on_mail():
        #
        #     return True

        c.set_on_connect_callback(on_connect)
        # c.set_on_mail_callback(on_mail)

        c.run('localhost', 9000, 'test_zmsg_types')
        c.wait_until_connected(2000)

        self.assertTrue(c.is_registered_for('STRING_VAR'))
        self.assertTrue(c.is_registered_for('DOUBLE_VAR'))
        self.assertTrue(c.is_registered_for('BINARY_VAR'))
        t = pymoos.time()

        self.assertTrue(c.notify('STRING_VAR', s, t))
        self.assertTrue(c.notify('DOUBLE_VAR', d, t))
        self.assertTrue(c.notify_binary('BINARY_VAR', str(x), t))

        time.sleep(1)

        idx = 0
        logger.debug('idx hit')
        msgs = c.fetch()
        logger.debug('fetch hit')
        self.assertNotEqual(len(msgs), 0)
        self.assertEqual(len(msgs), 3)
        for msg in msgs:
            logger.debug('iter hit')
            self.assertEqual(msg.time(), t)
            logger.debug('time hit')
            if msg.key() == 'STRING_VAR':
                logger.debug('string hit')
                self.assertTrue(msg.is_string())
                self.assertEqual(msg.string(), s)
                self.assertFalse(msg.is_double())
                self.assertFalse(msg.is_binary())
            elif msg.key() == 'DOUBLE_VAR':
                logger.debug('double hit')
                self.assertTrue(msg.is_double())
                self.assertEqual(msg.double(), d)
                self.assertFalse(msg.is_string())
                self.assertFalse(msg.is_binary())
            elif msg.key() == 'BINARY_VAR':
                logger.debug('binary hit')
                self.assertTrue(msg.is_binary())
                self.assertEqual(str(msg.binary_data()), str(x))
                self.assertTrue(msg.is_string())
                self.assertFalse(msg.is_double())

            idx += 1
            logger.debug('idx++ hit')

        self.assertEqual(idx, 3)

        c.close(True)



if __name__ == '__main__':
    unittest.main()
