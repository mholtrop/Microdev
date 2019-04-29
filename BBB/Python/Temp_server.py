#!/usr/bin/env python
import sys
import logging
import argparse
import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output, State

import plotly.graph_objs as go

import pandas as pd
import MySQLdb
from datetime import datetime,timedelta

from random import random

DB_USER ="maurik"
DB_PASSW= "wakende"
# DB_HOST="127.0.0.1"
DB_HOST="10.0.0.135"
DATABASE="sensors"
DB_PORT=3306        # set to 33306 to tunnel

parser = argparse.ArgumentParser(description='Temperature Logger web server using Dash.')
parser.add_argument('-d','--debug',action='count',help='Increate debug level',default=0);
parser.add_argument('-S','--slider',action='store_true',help='A range slider at the bottom of the graphs.')
parser.add_argument('-i','--interval',type=int,help='Interval wait time in seconds. def=60',default=60)
parser.add_argument('-p','--port',type=int,help='port for http, default 8050',default=8050)
args = parser.parse_args(sys.argv[1:])

# Col name   Title   Color  Table-name  y-axis
#
data_sets=[
        {'name':"Temp1" ,'title':"Out Temp" ,'table':"outdoor_tph" ,'column':"temp"    ,'graph':'y3','color':"#10FFD0"},
        {'name':"Hum1"  ,'title':"Out Humi" ,'table':"outdoor_tph" ,'column':"humidity",'graph':'y2','color':"#30D080"},
        {'name':"Press1",'title':"Out Press",'table':"outdoor_tph" ,'column':"pressure",'graph':'y1','color':"#9030D0"},
        {'name':"Temp2" ,'title':"In Temp"  ,'table':"basement_tph",'column':"temp"    ,'graph':'y3','color':"#1090FF"},
        {'name':"Hum2"  ,'title':"In Humi"  ,'table':"basement_tph",'column':"humidity",'graph':'y2','color':"#3080D0"},
        {'name':"Press2",'title':"In Press" ,'table':"basement_tph",'column':"pressure",'graph':'y1','color':"#9030FF"},
        {'name':"Temp3" ,'title':"CL Temp"  ,'table':"closet_th",   'column':"temp"    ,'graph':'y3','color':"#10FFFF"},
        {'name':"Hum3"  ,'title':"CL Humi"  ,'table':"closet_th",   'column':"humidity",'graph':'y2','color':"#20D0D0"}
        ]

data_columns =  {} #['time', 'temp', 'pressure', 'humidity']   # Do this by hand to the order is what you expect.
for t in data_sets:
    if not t['table'] in data_columns:
        data_columns[t['table']]=['time']
    data_columns[t['table']].append(t['column'])

data_store = {}

for d in data_columns:
        if args.debug > 0: print("Data columns {} are {}".format(d,data_columns[d]))
        data_store[d]=(pd.DataFrame(columns=data_columns[d]))

#
# Use the information from the table to create a Div content that shows the value for the latest reading.
#
div_value_labels_contents=[]
for t in data_sets:
    value_box_style = {'float':'left','width':'100px','height':'35px','text-align':'center','line-height':'35px','font-size':'24px','background':t['color']}
    text_box_style  = {'float':'left','width':'70px','height':'35px','text-align':'left','line-height':'35px','font-size':'14px','color':t['color']}
    div_value_labels_contents.append(html.Div(id=t['name'],children=t['name'],style=value_box_style))
    div_value_labels_contents.append(html.Div(id=t['name']+"tag",children=[t['title']],style=text_box_style))

div_value_labels_contents.append(".") # This dot at the end makes sure it displays properly. ## FIXME: is this really needed?

app = dash.Dash(__name__)
app.layout = html.Div([
        html.Div([
            html.Div(
                dcc.Slider(
                    id="Buff_slider",min=1./60.,max=1000.,step=1.,
                    marks={i:'{}.'.format(i) for i in range(0,1000,100)}),
                style={'height':'50px','width':'70%','float':'left'}),
            html.Div(dcc.Input(id="Buff_value",type="text",style={'width':'80px'}),style={'float':'left','width':'90px','height':'50px'}),
            html.Div(html.Button('Set Buffer Depth', id='buffer_depth_set_value',style={'background':'#B0C0FF'}),style={'height':'50px'})]),
    html.Div(
        [html.Div(dcc.Checklist(id="checklist-choice",
                    options=[{'label': 'Auto Update Plots ', 'value': 'AUP'}],
                    values=['AUP'], labelStyle={'display': 'inline-block'}),
                    style={'height':'30px','width':'300px','float':'left'}),
        html.Div(["Graph F:",dcc.Input(id="Graph_update_value",type="number",value=args.interval,style={'width':'100px'})],style={'height':'30px'})
        ]),
    html.Div(div_value_labels_contents,style={'height':'72px'}),
    dcc.Graph(id='Temps'),
    dcc.Interval(id='graph-update-timer', interval=args.interval*1000, n_intervals=0),
#    dcc.Interval(id='data-update-timer', interval=args.interval*1000, n_intervals=0),
    html.Div(id='data_store',style={'display':'none'})
    ]) # html.Div()

if args.slider:
    xaxis_style= {
    "rangeslider": {
         "autorange": True,
         "range": t_range
         },
     "type": "date"
     }
else:
    xaxis_style= {
        "type": "date"
     }

env_layout = go.Layout(
    title='Environtmental Data',
    #width=600,
    height=800,
    xaxis = xaxis_style,
    yaxis1=dict(
        title='Pressure',
        titlefont=dict(
            color='#9030D0'),
        tickfont=dict(
            color='#9030D0'),
        side='left',
        autorange=True,
        domain=[0.,0.33]),
    yaxis2=dict(
        title='Humidity',
        titlefont=dict(
            color='#30D090'),
        tickfont=dict(
            color='#30D090'),
        side='left',
        autorange=True,
        domain=[0.34,0.66]),
    yaxis3=dict(
        title='Temperature',
        titlefont=dict(
            color='#3090D0'),
        tickfont=dict(
            color='#3090D0'),
        side='left',
        autorange=True,
        domain=[0.67,1.]),

        #anchor='free',
        # overlaying='y'
        )

###### Generate call-backs for each of the labels with the data in them.
#
# This is a whee bit tricky stuff. This is a function that returns a unique function
# for each call, made unique because of the table_name and column_name that it is supplied with.
# The function that is returned is a callback, which returns the latest value from the
# data_store for table_name column column_name. This callback is "tied" to the appropriate input and output
# of the app.
#
# Function generator:
def Generate_update_value(table_name,column_name):
    def update_value(n_int,check_val,n_clicks):
        '''Callback to Update the text box values with the latest from the database'''
        # global data_store
        # if len(data_store[table_name]['time'])==0:
        #     if args.debug>0:print("Callback: update value {}.{} was zero.".format(table_name,column_name))
        #     update_data(1,None,0,48)
        #     return("n/a"+column_name)
        # else:
        #     idxmax=data_store[table_name]['time'].idxmax()
        #     return("{:5.2f}".format(data_store[table_name][column_name][idxmax]))
#        try:
        if args.debug>1: print("Update value for {} item {}".format(table_name,column_name))
        database= MySQLdb.connect(DB_HOST,DB_USER,DB_PASSW,DATABASE,DB_PORT)
        sql = "select {} from {} ORDER BY idx DESC LIMIT 1;".format(column_name,table_name)  # This selects the LAST entry in the table.
        cur=database.cursor()
        cur.execute(sql)
        vals=cur.fetchone()
        return("{:5.2f}".format(vals[0]))
#        except e:
#            print("Error occurred fetching values",e)
#            return("Err")

    return(update_value)

for t in data_sets:
    app.callback(Output(t['name'],'children'),
    [Input('graph-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])(
    Generate_update_value(t['table'],t['column']))



@app.callback(Output('Buff_value','value'),[Input('Buff_slider','value')])
def update_textarea(value):
    # Upate the text of the input box.
    try:
        val = float(value)
    except:
        val=24*7
    return('{:7.1f}'.format(val))

@app.callback(Output('Buff_slider','value'),[Input('buffer_depth_set_value','n_clicks')],[State('Buff_value','value')])
def update_slidervalue(dummy,value):
    if value is None:
        value=24*7

    return(value)

# @app.callback(Output('data-update-timer','interval'),[Input('Data_update_value','value')])
# def update_data_interval(value):
#     return(value*1000)         # Every N seconds update the data store.

@app.callback(Output('graph-update-timer','interval'),[Input('checklist-choice','values'),Input('Graph_update_value','value')])
def interval_control(check_list,value):
    if 'AUP' in check_list:
        return(value*1000)     # Every N seconds update the graphs.
    else:
        return(24*60*60*1000)  # Once per day  update graph

# @app.callback(Output('data_store','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')],[State('Buff_value','value')])
def update_data(interval,checklist_choice,n_clicks,buffer_depth):
    # select * from outdoor_tph as o, basement_tph as b where abs(cast(o.time as signed) -cast(b.time as signed))< 100 and  o.time > date_sub(now(),INTERVAL 6 HOUR)'

    global data_store

    if args.debug>0:print("Update data, buffer_depth: {} ".format(buffer_depth))

    if buffer_depth is None:
        dt_hours=24*7
    else:
        dt_hours=float(buffer_depth)

    first_time={}
    delta_time={}
    last_time={}
    for key in data_store:
        if data_store[key].size>10:
            first_time[key] = data_store[key]['time'][data_store[key]['time'].idxmin()]
            last_time[key]  = data_store[key]['time'][data_store[key]['time'].idxmax()]
            if data_store[key]['time'].idxmax()>2:
                delta_time[key] = data_store[key]['time'][data_store[key]['time'].idxmax()]-data_store[key]['time'][data_store[key]['time'].idxmax()-1]
            else:
                delta_time[key] = 1./60.



    if args.debug>0: print("Put data in data_store from DB")
#        try:
    database= MySQLdb.connect(DB_HOST,DB_USER,DB_PASSW,DATABASE,DB_PORT)

    for key in data_store:
        #data_store[key]=data_store[key].append(pd.read_sql_query(sql1,database),ignore_index=True)
        if data_store[key].size < 10:  # (near) Empty data store. Get a whole list from the DB.
            if args.debug>0: print("Fresh get of data for {}".format(key))
            sql1="select {} from {} where time > date_sub(now(),INTERVAL {} HOUR)".format(",".join(data_store[key].columns),key,dt_hours)
            data_store[key]=pd.read_sql_query(sql1,database)
            if args.debug>0: print("Initial DB fetch done, {} size is now {}".format(key,data_store[key].size))

        elif (first_time[key] > last_time[key] - timedelta(dt_hours/24.)+delta_time[key]): # Larger buffer was requested, so backfill.
            if args.debug>0: print("Data backfill for {}",key)
            sql1="select {} from {} where time > '{}' and time < '{}' ".format(",".join(data_store[key].columns),key,last_time[key] - timedelta(dt_hours/24.),first_time[key])
            extra_data = pd.read_sql_query(sql1,database)
            if extra_data.size > 0:
                data_store[key]=pd.concat([extra_data,data_store[key]],ignore_index=True)
            if args.debug>0: print("Backfull done, {} size is now {}".format(key,data_store[key].size))

        elif datetime.now() > last_time[key] + delta_time[key]: # There should be new data available, so get it.
            sql1="select {} from {} where time > '{}' ".format(",".join(data_store[key].columns),key,last_time[key])
            extra_data = pd.read_sql_query(sql1,database)
            if extra_data.size > 0:
                data_store[key]=data_store[key].append(extra_data,ignore_index=True)
                if args.debug>0: print("Data extension done, {} size is now {}".format(key,data_store[key].size))

    database.close()

    # See if we need to drop some data:
    for key in data_store:
        if key in last_time:
            if args.debug>0: print("key: {} last_time:".format(key),last_time[key])
            new_first = pd.Timestamp(last_time[key]) - timedelta(dt_hours/24.)
            if args.debug>0: print("new_first:",new_first)
            drop_list=data_store[key][(data_store[key]['time']<new_first)].index
            if len(drop_list)>0:
                data_store[key]=data_store[key].drop(drop_list)
                if args.debug>0: print("Done prune, {} size is now {}".format(key,data_store[key].size))

    return('OK')

@app.callback(Output('Temps','figure'),
        [Input('graph-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')],[State('Buff_value','value')])
def update_graph(interval,checklist_choice,n_clicks,buffer_depth_value):
    # select * from outdoor_tph as o, basement_tph as b where abs(cast(o.time as signed) -cast(b.time as signed))< 100 and  o.time > date_sub(now(),INTERVAL 6 HOUR)'

    if args.debug>0:print("Update graph, buffer_depth: {} ".format(buffer_depth_value))

    update_data(interval,checklist_choice,n_clicks,buffer_depth_value)

    db_dat=[]
    for s in data_sets:
        rows = data_store[s['table']]
        db_dat.append(
            go.Scatter(
                x=rows['time'],
                y=rows[s['column']],
                line=dict(
                    color = s['color'],
                    width = 2),
                name=s['title'],
                yaxis=s['graph'])
        )

    return( go.Figure(data=db_dat,layout=env_layout))

server = app.server # This is for the web server version with Apache2

if __name__ == '__main__':
    log = logging.getLogger('werkzeug')
    if args.debug>0:
        log.setLevel(logging.ERROR)
    else:
        log.setLevel(logging.DEBUG)

    print("Starting server on port: {}".format(args.port))
    app.run_server(debug=(args.debug>0),port=args.port)
