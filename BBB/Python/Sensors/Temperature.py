#!/usr/bin/env python
#
import sys
import argparse
import time
from datetime import datetime, timedelta
import MySQLdb
import zmq

DB_USER = "maurik"
DB_PASSW = "wakende"
DB_HOST = "127.0.0.1"
DATABASE = "sensors"
DB_PORT = 3306        # set to 33306 to tunnel

ZMQ_PORT = 5555
ZMQ_TIMEOUT = 1000

try:
    import paho.mqtt.client as mqtt
    MQTT_Broker = "10.0.0.131"  # Pimaker1
    MQTT_Port = 1883

except ImportError as e:
   print("Could not start MQTT")
   MQTT_Broker = None
   MQTT_Port = 0

SLEEP_TIME = 10       # Sleep time in seconds. This equals (almost) the 0MQ response time.
READ_INTERVAL = 10*60  # once per 10 minutes.
DEBUG = 0

try:
    from DevLib import BME280
except ImportError as e:
    sys.path.append('/home/maurik/Microdev/Python')
    try:
        from DevLib import BME280
    except ImportError:
        print("Please add DevLib to the PYTHONPATH")
        print(e)
        sys.exit()

try:
    from DevLib import SI7021
except ImportError as e:
    sys.path.append('/home/maurik/Microdev/Python')
    try:
        from DevLib import SI7021
    except ImportError:
        print("Please add DevLib to the PYTHONPATH")
        print(e)
        sys.exit()


def test_print(t1, p1, h1, t2, p2, h2):
    print("      Indoor (x76)  Outdoor (x77)")
    print("Temp:  {:>10.3f} {:>10.3f}".format(t1, t2))
    print("Pres:  {:>10.3f} {:>10.3f}".format(p1, p2))
    print("Humi:  {:>10.3f} {:>10.3f}".format(h1, h2))


def make_sql_table(curs, name, descr, has_press=True):
    """Create a blank temperature/pressure/humidity table with name.
       Register the new table in the index as well."""
    if DEBUG:
        print("Creating new temp/press/humi table: {}".format(name))

    sql = "select * from sensindex where name='{name}';".format(name=name)
    num = curs.execute(sql)
    if DEBUG > 1:
        print("Table {} has num={}".format(name, num))

    if num > 0:
        if DEBUG:
            print("Deleting the existing table.")
        try:
            sql = "delete from sensindex where name='{name}'".format(name=name)
            curs.execute(sql)

            sql = "DROP TABLE IF EXISTS {0}".format(name)
            curs.execute(sql)
        except MySQLdb.Error as ex:
            print("Error dropping table {0} = {1} - {2}".format(name, ex.args[0], ex.args[1]))
            return

    if has_press:
        sql = """
        CREATE TABLE IF NOT EXISTS {0} (
        idx int NOT NULL AUTO_INCREMENT,
        time timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
        temp float,
        pressure float,
        humidity float,
        PRIMARY KEY (idx) );
        """.format(name)
    else:
        sql = """
         CREATE TABLE IF NOT EXISTS {0} (
         idx int NOT NULL AUTO_INCREMENT,
         time timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
         temp float,
         humidity float,
         PRIMARY KEY (idx) );
         """.format(name)

    try:
        curs.execute(sql)
    except MySQLdb.Error as ex:
        print("Error creating table {0} = {1} - {2}".format(name, ex.args[0], ex.args[1]))
        return

    sql = """
    INSERT INTO sensindex (name,type,descr) VALUES ('{name}',{type},'{descr}')
    """.format(name=name, type=1, descr=descr)

    try:
        curs.execute(sql)
    except MySQLdb.Error as ex:
        print("Error inserting table into sensindex: {0} = {1} - {2}".format(name, ex.args[0], ex.args[1]))
        return


def insert_into_tph_table(curs, name, t, p, h):
    """Insert the temp, pressure and humidity into the tph table name"""
    sql = "insert into {name} (temp,pressure,humidity) VALUES ({t},{p},{h})".format(name=name, t=t, p=p, h=h)
    try:
        curs.execute(sql)
    except MySQLdb.Error as ex:
        print("Error inserting into table {0} = {1} - {2}".format(name, ex.args[0], ex.args[1]))
        return


def insert_into_th_table(curs, name, t, h):
    """Insert the temp and humidity into the th table name"""
    sql = "insert into {name} (temp,humidity) VALUES ({t},{h})".format(name=name, t=t, h=h)
    try:
        curs.execute(sql)
    except MySQLdb.Error as ex:
        print("Error inserting into table {0} = {1} - {2}".format(name, ex.args[0], ex.args[1]))
        return


def main(argv=None):
    """Main code."""

    global DEBUG

    if argv is None:
        argv = sys.argv

    parser = argparse.ArgumentParser(description='Temperature Logger.')
    parser.add_argument('-d', '--debug', action='count', help='Increate debug level', default=0)
    parser.add_argument('-C', '--create', action='store_true', help='Create the tables anew.')
    parser.add_argument('-i', '--interval', type=int, help='Interval wait time in seconds. def=600',
                        default=READ_INTERVAL)
    args = parser.parse_args(argv[1:])
    DEBUG = args.debug

    if DEBUG:
        print("Current libzmq version is %s" % zmq.zmq_version())
        print("Current  pyzmq version is %s" % zmq.__version__)

    try:
        database = MySQLdb.connect(DB_HOST, DB_USER, DB_PASSW, DATABASE, DB_PORT)
        curs = database.cursor()

        if args.create:
            make_sql_table(curs, "basement_tph", "Indoor BME280 at 0x76, below LEDBall lamp.")
            make_sql_table(curs, "outdoor_tph", "Outdoor BME280 at 0x76, in front of house.")
            database.commit()
    except Exception as ex:
        print("Issue with databse.")
        print(ex)
        sys.exit()

    if args.create:
        make_sql_table(curs, "basement_tph", "Indoor BME280 at 0x76, below LEDBall lamp.")
        make_sql_table(curs, "outdoor_tph", "Outdoor BME280 at 0x76, in front of house.")
        make_sql_table(curs, "closet_th", "Indoor SI7021 at 0x40, in pump closet.", has_press=False)
        database.commit()

    bme1 = BME280(0x76, 1)
    bme2 = BME280(0x77, 1)
    bme1.Configure()
    bme2.Configure()
    oversample_settings = (16, 16, 16)
    bme1.Set_Oversampling(oversample_settings)
    bme2.Set_Oversampling(oversample_settings)

    (t1, p1, h1) = bme1.Read_Data()
    (t2, p2, h2) = bme2.Read_Data()
    p1_save = p1
    p2_save = p2

    si = SI7021(1)

# Setup the 0MQ socket:
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    try:
        socket.bind("tcp://*:" + str(ZMQ_PORT))
    except Exception as ex:
        print("Error binding to {}\n".format(ZMQ_PORT))
        print(ex)
        raise

    tock = datetime.now()

    while True:

        events = socket.poll(timeout=ZMQ_TIMEOUT)

        if events > 0:
            now = time.time()
            message = socket.recv()
            dt1 = time.time()
            if args.debug:
                print("We got a message: ", message)

            if message[0:1] == b'i' or message[0:1] == b'b':  # Indoor or basement information
                tph = ['i'] + list(bme1.Read_Data())
            elif message[0:1] == b'o':  # Outdoor temp.
                tph = ['o'] + list(bme2.Read_Data())
            elif message[0:1] == b'c':  # Closet temp.
                tph = ['c'] + list(si.Read_Humi_Temp())
            elif message[0:1] == b'a':  # Return all measurements: indoor, outdoor, closet
                # Note: All the time spend to reply is in the following reads from the SMBus: 0.1 + 0.1 + 0.4 seconds.
                tph1 = bme1.Read_Data()
                tph2 = bme2.Read_Data()
                tph3 = si.Read_Humi_Temp()
                tph = ['i'] + list(tph1) +\
                      ['o'] + list(tph2) +\
                      ['c'] + list(tph3)
            else:
                tph = ['x', -99, -99, -99]

            if args.debug:
                print("Returning message: ", tph)

            socket.send_json(tph)

        tick = datetime.now()
        if (tick - tock).seconds > args.interval:
            tock = tick

            try:
                database = MySQLdb.connect(DB_HOST, DB_USER, DB_PASSW, DATABASE, DB_PORT)
                curs = database.cursor()
                if oversample_settings != bme1.Get_Oversampling():  # Power glitch? Reconfigure
                    bme1.Configure()
                    bme1.Set_Oversampling(oversample_settings)
                if oversample_settings != bme2.Get_Oversampling():  # Power glitch? Reconfigure
                    bme2.Configure()
                    bme2.Set_Oversampling(oversample_settings)

                (t1, p1, h1) = bme1.Read_Data()
                (t2, p2, h2) = bme2.Read_Data()

                if abs(p1 - p1_save) > 200:     # Large pressure change (drop). Probably a power fail so reconfigure.
                    bme1.Configure()
                    bme1.Set_Oversampling(oversample_settings)
                    continue
                if abs(p2 - p2_save) > 200:
                    bme2.Configure()
                    bme2.Set_Oversampling(oversample_settings)
                    continue

                p1_save = p1
                p2_save = p2

                if DEBUG > 1:
                    test_print(t1, p1, h1, t2, p2, h2)

                insert_into_tph_table(curs, "basement_tph", t1, p1, h1)
                insert_into_tph_table(curs, "outdoor_tph", t2, p2, h2)
                database.commit()

                (h1, t1) = si.Read_Humi_Temp()
                insert_into_th_table(curs, "closet_th", t1, h1)
                database.commit()

                database.close()

            except Exception as ex:
                print("Error occurred in main while loop.")
                print(ex)


if __name__ == '__main__':
    main()
