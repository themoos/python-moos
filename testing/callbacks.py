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
    def test_on_connect(self):
        c = pymoos.comms()
        c.set_on_connect_callback(
                lambda : self.assertTrue(c.register('SIMPLE_VAR', 0)))
        c.run('localhost', 9000, 'test_on_connect')
        c.wait_until_connected(2000)

        self.assertTrue(c.is_registered_for('SIMPLE_VAR'))

        c.close(True)

    def test_on_connect_1(self):
        c = pymoos.comms()

        def on_connect():
            self.assertTrue(c.register('SIMPLE_VAR', 0))
            return True

        c.set_on_connect_callback(on_connect)
        c.run('localhost', 9000, 'test_on_connect_1')
        c.wait_until_connected(2000)

        self.assertTrue(c.is_registered_for('SIMPLE_VAR'))

        c.close(True)

    def test_on_mail(self):
        c = pymoos.comms()
        received_mail = False

        def on_connect():
            self.assertTrue(c.register('ONMAIL_VAR', 0))
            return True

        def on_new_mail():
            nonlocal received_mail
            for msg in c.fetch():
                logger.debug('on_new_mail hit')
                self.assertTrue(msg.is_name('ONMAIL_VAR'))
                self.assertEqual(msg.double(), 1)
                received_mail = True
                logger.debug('received_mail')
            return True

        c.set_on_connect_callback(on_connect)
        c.set_on_mail_callback(on_new_mail)
        c.run('localhost', 9000, 'test_on_mail')
        c.wait_until_connected(2000)

        self.assertTrue(c.is_registered_for('ONMAIL_VAR'))
        self.assertFalse(received_mail)
        self.assertTrue(c.notify('ONMAIL_VAR', 1, -1))

        time.sleep(1)

        self.assertTrue(received_mail)

        c.close(True)

    def test_on_mail_queues(self):
        c = pymoos.comms()
        received_mail = False
        received_mail_q_v1 = False
        received_mail_q_v2 = False
        received_mail_q2_v = False

        def on_connect():
            self.assertTrue(c.register('ONMAIL_VAR', 0))
            self.assertTrue(c.register('ONQUEUE_VAR1', 0))
            self.assertTrue(c.register('ONQUEUE_VAR2', 0))
            self.assertTrue(c.register('ONQUEUE2_VAR', 0))
            return True

        def on_new_mail():
            nonlocal received_mail
            for msg in c.fetch():
                logger.debug('on_new_mail hit')
                self.assertTrue(msg.is_name('ONMAIL_VAR'))
                self.assertEqual(msg.double(), 1)
                received_mail = True
                logger.debug('received_mail')
            return True

        def queue1(msg):
            nonlocal received_mail_q_v1, received_mail_q_v2
            if msg.is_name('ONQUEUE_VAR1'):
                self.assertEqual(msg.double(), 2)
                received_mail_q_v1 = True
            elif msg.is_name('ONQUEUE_VAR2'):
                self.assertEqual(msg.double(), 3)
                received_mail_q_v2 = True
            return True

        def queue2(msg):
            nonlocal received_mail_q2_v
            if msg.is_name('ONQUEUE2_VAR'):
                self.assertEqual(msg.double(), 4)
                received_mail_q2_v = True
            return True



        c.set_on_connect_callback(on_connect)
        c.set_on_mail_callback(on_new_mail)
        c.add_active_queue('queue1', queue1)
        c.add_message_route_to_active_queue('queue1', 'ONQUEUE_VAR1')
        c.add_message_route_to_active_queue('queue1', 'ONQUEUE_VAR2')
        c.add_active_queue('queue2', queue2)
        c.add_message_route_to_active_queue('queue2', 'ONQUEUE2_VAR')
        c.run('localhost', 9000, 'test_on_mail_queues')
        c.wait_until_connected(5000)

        self.assertTrue(c.is_registered_for('ONMAIL_VAR'))
        self.assertTrue(c.is_registered_for('ONQUEUE_VAR1'))
        self.assertTrue(c.is_registered_for('ONQUEUE_VAR2'))
        self.assertTrue(c.is_registered_for('ONQUEUE2_VAR'))
        self.assertFalse(received_mail)
        self.assertFalse(received_mail_q_v1)
        self.assertFalse(received_mail_q_v2)
        self.assertFalse(received_mail_q2_v)
        self.assertTrue(c.notify('ONMAIL_VAR', 1, -1))
        self.assertTrue(c.notify('ONQUEUE_VAR1', 2, -1))
        self.assertTrue(c.notify('ONQUEUE_VAR2', 3, -1))
        self.assertTrue(c.notify('ONQUEUE2_VAR', 4, -1))

        time.sleep(1)

        self.assertTrue(received_mail)
        self.assertTrue(received_mail_q_v1)
        self.assertTrue(received_mail_q_v2)
        self.assertTrue(received_mail_q2_v)

        c.close(True)




if __name__ == '__main__':
    unittest.main()
