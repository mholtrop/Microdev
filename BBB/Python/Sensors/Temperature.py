#!/usr/bin/env python
#
import sys
import argparse
import time
import datetime
import MySQLdb

DB_USER ="maurik"
DB_PASSW= "wakende"
DB_HOST="127.0.0.1"
DATABASE="sensors"
DB_PORT=3306        # set to 33306 to tunnel

READ_INTERVAL=10*60 # once per 10 minutes.

DEBUG=0

try:
    from DevLib import BME280
except ImportError as e:
    sys.path.append('/home/maurik/Microdev/BBB/Python')
    try:
        from DevLib import BME280
    except ImportError:
        print("Please add DevLib to the PYTHONPATH")
        print(e)
        sys.exit()

def TestPrint(t1,p1,h1,t2,p2,h2):
    print("      Indoor (x76)  Outdoor (x77)")
    print("Temp:  {:>10.3f} {:>10.3f}".format(t1,t2))
    print("Pres:  {:>10.3f} {:>10.3f}".format(p1,p2))
    print("Humi:  {:>10.3f} {:>10.3f}".format(h1,h2))

def MakeSQLTable(curs,name,descr):
    """Create a blank temperature/pressure/humidity table with name.
       Register the new table in the index as well."""
    if DEBUG: print("Creating new temp/press/humi table: {}".format(name))

    sql = "select * from sensindex where name='{name}';".format(name=name)
    num = curs.execute(sql)
    if DEBUG>1: print("Table {} has num={}".format(name,num))

    if num>0:
        if DEBUG: print("Deleting the existing table.")
        try:
            sql = "delete from sensindex where name='{name}'".format(name=name)
            curs.execute(sql)

            sql = "DROP TABLE IF EXISTS {0}".format(name)
            curs.execute(sql)
        except MySQLdb.Error as e:
            print("Error dropping table {0} = {1} - {2}".format(name,e.args[0],e.args[1]))
            return


    sql = """
    CREATE TABLE IF NOT EXISTS {0} (
    idx int NOT NULL AUTO_INCREMENT,
    time timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    temp float,
    pressure float,
    humidity float,
    PRIMARY KEY (idx) );
    """.format(name)

    try:
        curs.execute(sql)
    except MySQLdb.Error as e:
        print("Error creating table {0} = {1} - {2}".format(name,e.args[0],e.args[1]))
        return

    sql = """
    INSERT INTO sensindex (name,type,descr) VALUES ('{name}',{type},'{descr}')
    """.format(name=name,type=1,descr=descr)

    try:
        curs.execute(sql)
    except MySQLdb.Error as e:
        print("Error inserting table into sensindex: {0} = {1} - {2}".format(name,e.args[0],e.args[1]))
        return

def Insert_into_tph_table(curs,name,t,p,h):
    """Insert the temp, pressure and humidity into the tph table name"""
    sql = "insert into {name} (temp,pressure,humidity) VALUES ({t},{p},{h})".format(name=name,t=t,p=p,h=h)
    try:
        curs.execute(sql)
    except MySQLdb.Error as e:
        print("Error inserting into table {0} = {1} - {2}".format(name,e.args[0],e.args[1]))
        return


def main(argv=None):
    '''Main code.'''

    global DEBUG

    if argv is None:
        argv = sys.argv

    parser = argparse.ArgumentParser(description='Temperature Logger.')
    parser.add_argument('-d','--debug',action='count',help='Increate debug level',default=0);
    parser.add_argument('-C','--create',action='store_true',help='Create the tables anew.')
    parser.add_argument('-i','--interval',type=int,help='Interval wait time in seconds. def=600',default=600)
#    parser.add_argument('-H','--home',type=str,help='Directory to treat as home. Default: $HOME/nest');
#    parser.add_argument('-u','--until',type=str,help='Run until this date-time, as in 2015-03-19 03:10:11 (Format=%%Y-%%m-%%d %%X)');
#    parser.add_argument('-t','--time',type=int,help='Time: Run until time seconds have elapsed. default=0',default=0);
#    parser.add_argument('-i','--interval',type=int,help='Interval: Number of seconds between reads, default={0}'.format(READ_INTERVAL),default=READ_INTERVAL);
#    parser.add_argument('-r','--roentgen',action='store_true',help="Connect to roentgen DB over tunnel")
    args = parser.parse_args(argv[1:])
    DEBUG = args.debug

    try:
        database= MySQLdb.connect(DB_HOST,DB_USER,DB_PASSW,DATABASE,DB_PORT)
        curs    = database.cursor()

        if args.create:
            MakeSQLTable(curs,"basement_tph","Indoor BME280 at 0x76, below LEDBall lamp.")
            MakeSQLTable(curs,"outdoor_tph","Outdoor BME280 at 0x76, in front of house.")
            database.commit()
    except Exception as e:
        print("Issue with databse.")
        print(e)
        sys.exit()


    bme1 = BME280(0x76,1)
    bme2 = BME280(0x77,1)
    bme1.Configure()
    bme2.Configure()
    Oversample_settings = (16,16,16)
    bme1.Set_Oversampling(Oversample_settings)
    bme2.Set_Oversampling(Oversample_settings)

    if args.create:
        MakeSQLTable(curs,"basement_tph","Indoor BME280 at 0x76, below LEDBall lamp.")
        MakeSQLTable(curs,"outdoor_tph","Outdoor BME280 at 0x76, in front of house.")
        database.commit()

    (t1,p1,h1)=bme1.Read_Data()
    (t2,p2,h2)=bme2.Read_Data()
    p1_save=p1
    p2_save=p2

    while True:
        try:
            database= MySQLdb.connect(DB_HOST,DB_USER,DB_PASSW,DATABASE,DB_PORT)
            curs    = database.cursor()
            if Oversample_settings != bme1.Get_Oversampling():  # Power glitch? Reconfigure
                bme1.Configure()
                bme1.Set_Oversampling(Oversample_settings)
            if Oversample_settings != bme2.Get_Oversampling():  # Power glitch? Reconfigure
                bme2.Configure()
                bme2.Set_Oversampling(Oversample_settings)

            (t1,p1,h1)=bme1.Read_Data()
            (t2,p2,h2)=bme2.Read_Data()

            if abs(p1 - p1_save)>200:     # Large pressure change (drop). Probably a power fail so reconfigure.
                bme1.Configure()
                bme1.Set_Oversampling(Oversample_settings)
                continue
            if abs(p2 - p2_save)>200:
                bme2.Configure()
                bme2.Set_Oversampling(Oversample_settings)
                continue

            p1_save=p1
            p2_save=p2

            if DEBUG>1:  TestPrint(t1,p1,h1,t2,p2,h2

            Insert_into_tph_table(curs,"basement_tph",t1,p1,h1)
            Insert_into_tph_table(curs,"outdoor_tph",t2,p2,h2)
            database.commit()
            database.close()
            time.sleep(args.interval)
        except Exception as e:
            print("Error occurred in main while loop.")
            print(e)

if __name__ == '__main__':
    main()
