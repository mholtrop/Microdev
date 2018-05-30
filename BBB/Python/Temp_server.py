import sys
import logging
import argparse
import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output, State, Event

import plotly.graph_objs as go

import pandas as pd
import MySQLdb
from datetime import datetime,timedelta

from random import random

try:
    from DevLib import BME280
except ImportError as e:
    print("Please add DevLib to the PYTHONPATH")
    print(e)

DB_USER ="maurik"
DB_PASSW= "wakende"
# DB_HOST="127.0.0.1"
DB_HOST="10.0.0.135"
DATABASE="sensors"
DB_PORT=3306        # set to 33306 to tunnel

parser = argparse.ArgumentParser(description='Temperature Logger.')
parser.add_argument('-d','--debug',action='count',help='Increate debug level',default=0);
parser.add_argument('-S','--slider',action='store_true',help='A range slider at the bottom of the graphs.')
parser.add_argument('-m','--mock',action='store_true',help='Use mock data for local sensors')
parser.add_argument('-i','--interval',type=int,help='Interval wait time in seconds. def=60',default=60)
parser.add_argument('-p','--port',type=int,help='port for http, default 8050',default=8050)
args = parser.parse_args(sys.argv[1:])

#     Col name   Title   Color  Store  y-axis
data_sets=[("temp"    ,"Outdoor Temp","#3090D0","outdoor_tph",'y1'),
        ("temp"    ,"Indoor Temp","#3090FF","basement_tph" ,'y1'),
        ("pressure","Outdoor Press","#9030D0","outdoor_tph",'y2'),
        ("pressure","Indoor Press","#9030FF","basement_tph",'y2'),
        ("humidity","Outdoor Humidity","#30D090","outdoor_tph",'y3'),
        ("humidity","Indoor Humidity","#30FF90","basement_tph",'y3')]

cols =  ['time', 'temp', 'pressure', 'humidity']   # Do this by hand to the order is what you expect.
data_store = {}
for ds in data_sets:
    if not ds[3] in data_store:
        data_store[ds[3]]=(pd.DataFrame(columns=cols))

#
# For reading the sensors directly.
#
Oversample_settings = (16,16,16)

def setup_sensors():
    '''Setup the sensors for reading. '''
    global bme1,bme2
    if not args.mock:
        bme1.Configure()
        bme2.Configure()
        bme1.Set_Oversampling(Oversample_settings)
        bme2.Set_Oversampling(Oversample_settings)
    return(0)

if not args.mock:
    try:
        bme1 = BME280(0x76,1)
        bme2 = BME280(0x77,1)
        setup_sensors()

    except:
        print("Could not allocate the sensors.")

value_box_style=[]
text_box_style=[]
for i in range(len(data_sets)):
    value_box_style.append({'float':'left','width':'100px','height':'35px','text-align':'center','line-height':'35px','font-size':'24px','background':data_sets[i][2]})
    text_box_style.append({'float':'left','width':'120px','height':'35px','text-align':'left','line-height':'35px','font-size':'14px','color':data_sets[i][2]})

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
                    options=[{'label': 'Auto Update Plots ', 'value': 'AUP'},{'label': 'Values from DB ', 'value': 'DB'}],
                    values=['AUP','DB'], labelStyle={'display': 'inline-block'}),
                    style={'height':'30px','width':'300px','float':'left'}),
        html.Div(["Graph Update Freq:",dcc.Input(id="Graph_update_value",type="number",value=args.interval,style={'width':'100px'})],style={'float':'left','width':'240px','height':'30px'}),
        html.Div(["Data Update Freq:",dcc.Input(id="Data_update_value",type="number",value=100,style={'width':'80px'})],style={'height':'30px'})
        ]),
    html.Div([
        html.Div(id="Temp1",children="temp1",style=value_box_style[0]),
        html.Div(id="Temp1tag",children=[data_sets[0][1]],style=text_box_style[0]),
        html.Div(id="Temp2",children="temp2",style=value_box_style[1]),
        html.Div(id="Temp2tag",children=[data_sets[1][1]],style=text_box_style[1]),
        html.Div(id="Press1",children="press1",style=value_box_style[2]),
        html.Div(id="Press1tag",children=[data_sets[2][1]],style=text_box_style[2]),
        html.Div(id="Press2",children="pres2",style=value_box_style[3]),
        html.Div(id="Press2tag",children=[data_sets[3][1]],style=text_box_style[3]),
        html.Div(id="Hum1",children="hum1",style=value_box_style[4]),
        html.Div(id="Hum1tag",children=[data_sets[4][1]],style=text_box_style[4]),
        html.Div(id="Hum2",children="hum2",style=value_box_style[5]),
        html.Div(id="Hum2tag",children=[data_sets[5][1]],style=text_box_style[5]),
        "."
        ],
    style={'height':'38px'}),
    dcc.Graph(id='Temps'),
    dcc.Interval(id='graph-update-timer', interval=args.interval*1000, n_intervals=0),
    dcc.Interval(id='data-update-timer', interval=args.interval*10000, n_intervals=0),
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
    yaxis=dict(
        title='Temperature',
        titlefont=dict(
            color='#3090D0'),
        tickfont=dict(
            color='#3090D0'),
        side='left',
        autorange=True,
        domain=[0.,0.33]),
    yaxis2=dict(
        title='Pressure',
        titlefont=dict(
            color='#9030D0'),
        tickfont=dict(
            color='#9030D0'),
        side='left',
        autorange=True,
        domain=[0.34,0.66]),
    yaxis3=dict(
        title='Humidity',
        titlefont=dict(
            color='#30D090'),
        tickfont=dict(
            color='#30D090'),
        side='left',
        autorange=True,
        domain=[0.67,1.])
        #anchor='free',
        # overlaying='y'
        )

@app.callback(Output('Temp1','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])
def update_value1(n_int,check_val,n_clicks):
    '''Update the text box values'''
    idxmax=data_store['outdoor_tph']['time'].idxmax()
    return("{:5.2f}".format(data_store['outdoor_tph']['temp'][idxmax]))

@app.callback(Output('Temp2','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])
def update_value2(n_int,check_val,n_clicks):
    '''Update the text box values'''
    idxmax=data_store['basement_tph']['time'].idxmax()
    return("{:5.2f}".format(data_store['basement_tph']['temp'][idxmax]))

@app.callback(Output('Press1','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])
def update_value3(n_int,check_val,n_clicks):
    '''Update the text box values'''
    idxmax=data_store['outdoor_tph']['time'].idxmax()
    return("{:6.1f}".format(data_store['outdoor_tph']['pressure'][idxmax]))

@app.callback(Output('Press2','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])
def update_value4(n_int,check_val,n_clicks):
    '''Update the text box values'''
    idxmax=data_store['basement_tph']['time'].idxmax()
    return("{:6.1f}".format(data_store['basement_tph']['pressure'][idxmax]))

@app.callback(Output('Hum1','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])
def update_value5(n_int,check_val,n_clicks):
    '''Update the text box values'''
    idxmax=data_store['outdoor_tph']['time'].idxmax()
    return("{:5.2f}".format(data_store['outdoor_tph']['humidity'][idxmax]))

@app.callback(Output('Hum2','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')])
def update_value6(n_int,check_val,n_clicks):
    '''Update the text box values'''
    idxmax=data_store['basement_tph']['time'].idxmax()
    return("{:5.2f}".format(data_store['basement_tph']['humidity'][idxmax]))


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

@app.callback(Output('data-update-timer','interval'),[Input('Data_update_value','value')])
def update_data_interval(value):
    return(value*1000)         # Every N seconds update the data store.

@app.callback(Output('graph-update-timer','interval'),[Input('checklist-choice','values'),Input('Graph_update_value','value')])
def interval_control(check_list,value):
    if 'AUP' in check_list:
        return(value*1000)     # Every N seconds update the graphs.
    else:
        return(24*60*60*1000)  # Once per day  update graph

@app.callback(Output('data_store','children'),[Input('data-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')],[State('Buff_value','value')])
def update_data(interval,checklist_choice,n_clicks,buffer_depth):
    # select * from outdoor_tph as o, basement_tph as b where abs(cast(o.time as signed) -cast(b.time as signed))< 100 and  o.time > date_sub(now(),INTERVAL 6 HOUR)'

    global data_store

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


    if 'DB' in checklist_choice:
        if args.debug>0: print("Put data in data_store from DB")
#        try:
        if True:
            database= MySQLdb.connect(DB_HOST,DB_USER,DB_PASSW,DATABASE,DB_PORT)

            for key in data_store:
                #data_store[key]=data_store[key].append(pd.read_sql_query(sql1,database),ignore_index=True)
                if data_store[key].size < 10:  # (near) Empty data store. Get a whole list from the DB.
                    if args.debug>0: print("Fresh get of data for {}",key)
                    sql1="select time,temp,pressure,humidity from {} where time > date_sub(now(),INTERVAL {} HOUR)".format(key,dt_hours)
                    data_store[key]=pd.read_sql_query(sql1,database)
                    if args.debug>0: print("Initial DB fetch done, {} size is now {}".format(key,data_store[key].size))

                elif (first_time[key] > last_time[key] - timedelta(dt_hours/24.)+delta_time[key]): # Larger buffer was requested, so backfill.
                    if args.debug>0: print("Data backfill for {}",key)
                    sql1="select time,temp,pressure,humidity from {} where time > '{}' and time < '{}' ".format(key,last_time[key] - timedelta(dt_hours/24.),first_time[key])
                    extra_data = pd.read_sql_query(sql1,database)
                    if extra_data.size > 0:
                        data_store[key]=pd.concat([extra_data,data_store[key]],ignore_index=True)
                    if args.debug>0: print("Backfull done, {} size is now {}".format(key,data_store[key].size))

                elif datetime.now() > last_time[key] + delta_time[key]: # There should be new data available, so get it.
                    sql1="select time,temp,pressure,humidity from {} where time > '{}' ".format(key,last_time[key])
                    extra_data = pd.read_sql_query(sql1,database)
                    if extra_data.size > 0:
                        data_store[key]=data_store[key].append(extra_data,ignore_index=True)
                        if args.debug>0: print("Data extension done, {} size is now {}".format(key,data_store[key].size))

            database.close()

        # except Exception as e:
        #     print("Issue with database(?)")
        #     print("SQL: {}".format(sql1))
        #     print(e)
        #     sys.exit()

    elif not args.mock:

        if Oversample_settings != bme1.Get_Oversampling() or Oversample_settings != bme2.Get_Oversampling():  # Power glitch? Reconfigure
            setup_sensors()

        (t1,p1,h1)=bme1.Read_Data()
        (t2,p2,h2)=bme2.Read_Data()
        time = pd.Timestamp(datetime.now())
        if args.debug>0: print("Sensor read: {} {} {} {} {} {} {}".format(time,t1,p1,h1,t2,p2,h2))
        # "basement_tph",t1,p1,h1
        # "outdoor_tph" ,t2,p2,h2
        # Add the data to the end of the store.
        data_store["basement_tph"]=data_store["basement_tph"].append(pd.DataFrame([[time,t1,p1,h1]],columns=['time', 'temp', 'pressure', 'humidity']),ignore_index=True)
        data_store["outdoor_tph"]=data_store["outdoor_tph"].append(pd.DataFrame([[time,t2,p2,h2]],columns=['time', 'temp', 'pressure', 'humidity']),ignore_index=True)
    else:
        (t1,p1,h1)=(20.+random()*2.,1000+random()*100,40.+random()*1.)
        (t2,p2,h2)=(15.+random()*8.,1000+random()*400,50.+random()*4.)
        time = pd.Timestamp(datetime.now())
        if args.debug>0: print("Sensor read: {} {} {} {} {} {} {}".format(time,t1,p1,h1,t2,p2,h2))
        # "basement_tph",t1,p1,h1
        # "outdoor_tph" ,t2,p2,h2
        # Add the data to the end of the store.
        data_store["basement_tph"]=data_store["basement_tph"].append(pd.DataFrame([[time,t1,p1,h1]],columns=['time', 'temp', 'pressure', 'humidity']),ignore_index=True)
        data_store["outdoor_tph"]=data_store["outdoor_tph"].append(pd.DataFrame([[time,t2,p2,h2]],columns=['time', 'temp', 'pressure', 'humidity']),ignore_index=True)

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



# Input('data_store','children')
@app.callback(Output('Temps','figure'),
        [Input('graph-update-timer','n_intervals'),Input('checklist-choice','values'),Input('buffer_depth_set_value','n_clicks')],[State('Buff_value','value')])
def update_graph(interval,checklist_choice,buffer_depth,buffer_depth_value):
    # select * from outdoor_tph as o, basement_tph as b where abs(cast(o.time as signed) -cast(b.time as signed))< 100 and  o.time > date_sub(now(),INTERVAL 6 HOUR)'

    if args.debug>0:print("Update graph.")

    db_dat=[]
    for s in data_sets:
        rows = data_store[s[3]]
        db_dat.append(
            go.Scatter(
                x=rows['time'],
                y=rows[s[0]],
                line=dict(
                    color = s[2],
                    width = 2),
                name=s[1],
                yaxis=s[4])
        )

    return( go.Figure(data=db_dat,layout=env_layout))

server = app.server # This is for the web server version with Apache2

if __name__ == '__main__':
    log = logging.getLogger('werkzeug')
    if args.debug>0:
        log.setLevel(logging.ERROR)
    else:
        log.setLevel(logging.DEBUG)

    app.run_server(debug=(args.debug>0),port=args.port)
