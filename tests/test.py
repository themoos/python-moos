#! /usr/bin/env python
"""
unittests for pymoos
"""

import unittest
import subprocess
import time
import logging
logging.basicConfig(level=logging.DEBUG,
        format='%(asctime)s | %(levelname)s:'
        ' %(name)s.%(funcName)s = %(message)s',
        )
logger = logging.getLogger('tests')
logger.level = logging.DEBUG

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

    def test_notify_msg(self):
        c = pymoos.comms()
        c.set_on_connect_callback(
                lambda : self.assertTrue(c.register('SIMPLE_VAR')))
        c.run('localhost', 9000, 'test_notify_msg')
        c.wait_until_connected(5000)
        time.sleep(1)

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

        c.set_on_connect_callback(on_connect)

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

    def test_on_connect_callback_inline(self):
        c = pymoos.comms()
        c.set_on_connect_callback(
                lambda : self.assertTrue(c.register('SIMPLE_VAR')))
        c.run('localhost', 9000, 'test_on_connect_callback_inline')
        c.wait_until_connected(2000)
        time.sleep(1)

        self.assertTrue(c.is_registered_for('SIMPLE_VAR'))

        c.close(True)

    def test_on_connect_callback(self):
        logger.debug(' on ')
        c = pymoos.comms()

        def on_connect():
            logger.debug(' on ')
            self.assertTrue(c.register('SIMPLE_VAR', 0))
            return True

        c.set_on_connect_callback(on_connect)
        c.run('localhost', 9000, 'test_on_connect_callback')
        c.wait_until_connected(2000)
        time.sleep(.1)

        self.assertTrue(c.is_registered_for('SIMPLE_VAR'))

        c.close(True)

    def test_on_mail_callback(self):
        logger.debug(' on ')
        c = pymoos.comms()
        received_mail = False

        def on_connect():
            logger.debug(' on ')
            self.assertTrue(c.register('ONMAIL_VAR', 0))
            return True

        def on_new_mail():
            logger.debug(' on ')
            nonlocal received_mail
            for msg in c.fetch():
                logger.debug(' one new mail ')
                self.assertTrue(msg.is_name('ONMAIL_VAR'))
                self.assertEqual(msg.double(), 1)
                received_mail = True
                logger.debug(' mail processed ')
            return True

        c.set_on_connect_callback(on_connect)
        c.set_on_mail_callback(on_new_mail)
        c.run('localhost', 9000, 'test_on_mail_callback')
        c.wait_until_connected(2000)

        self.assertTrue(c.is_registered_for('ONMAIL_VAR'))
        self.assertFalse(received_mail)
        self.assertTrue(c.notify('ONMAIL_VAR', 1, -1))

        time.sleep(1)

        self.assertTrue(received_mail)

        c.close(True)

    def test_on_mail_active_queues(self):
        logger.debug(' on ')
        c = pymoos.comms()
        received_mail = False
        received_mail_q_v1 = False
        received_mail_q_v2 = False
        received_mail_q2_v = False

        def on_connect():
            logger.debug(' on ')
            self.assertTrue(c.register('ONMAIL_VAR', 0))
            self.assertTrue(c.register('ONQUEUE_VAR1', 0))
            self.assertTrue(c.register('ONQUEUE_VAR2', 0))
            self.assertTrue(c.register('ONQUEUE2_VAR', 0))
            return True

        def on_new_mail():
            logger.debug(' on ')
            nonlocal received_mail
            for msg in c.fetch():
                logger.debug(' one new mail ')
                self.assertTrue(msg.is_name('ONMAIL_VAR'))
                self.assertEqual(msg.double(), 1)
                received_mail = True
                logger.debug(' mail processed')
            return True

        def queue1(msg):
            logger.debug(' on ')
            nonlocal received_mail_q_v1, received_mail_q_v2
            if msg.is_name('ONQUEUE_VAR1'):
                self.assertEqual(msg.double(), 2)
                received_mail_q_v1 = True
            elif msg.is_name('ONQUEUE_VAR2'):
                self.assertEqual(msg.double(), 3)
                received_mail_q_v2 = True
            return True

        def queue2(msg):
            logger.debug(' on ')
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
        c.run('localhost', 9000, 'test_on_mail_active_queues')
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
