import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
#from tkinter import filedialog
import time
from serial.tools.list_ports import comports
import serial
from datetime import datetime
import os
import pymeasure
from pymeasure.instruments.keithley import Keithley6517B
import sys
import clr


#Configure references and add libraries for Thorlabs stages
# Path in "clr.AddReference" below can be changed to locaiton of .dll's if
# they are not in the default install location
clr.AddReference("C:\\Program Files\\Thorlabs\\Kinesis\\Thorlabs.MotionControl.DeviceManagerCLI.dll")
clr.AddReference("C:\\Program Files\\Thorlabs\\Kinesis\\Thorlabs.MotionControl.GenericMotorCLI.dll")
clr.AddReference("C:\\Program Files\\Thorlabs\\Kinesis\\ThorLabs.MotionControl.IntegratedStepperMotorsCLI.dll")
clr.AddReference("C:\\Program Files\\Thorlabs\\Kinesis\\ThorLabs.MotionControl.Benchtop.StepperMotorCLI.dll")
from Thorlabs.MotionControl.DeviceManagerCLI import *
from Thorlabs.MotionControl.GenericMotorCLI import *
from Thorlabs.MotionControl.IntegratedStepperMotorsCLI import *
from Thorlabs.MotionControl.Benchtop.StepperMotorCLI import *
from System import Decimal


#from all_stages import *

SIMULATION = True ########################################################


#bar??Pos format = [bar_x, bar_y, pd_x, pd_y, laser_rot, pd_rot]
barInPos = [75,75,75,75,45,45]
barOutPos = [0,0,0,0,0,0]

pdUnits = 'pA'
linUnits = 'mm'
rotUnits = '°'

fout = ''
foutHeader = 'timestamp\tbar_x\tbar_y\tpd_x\tpd_y\tlaser_rot\tpd_rot\tpd1\tpd2\tpd3\tbar in(2)/out(0)\n'




bkgColor = '#c5c9c7'

run = True
comConnect = False
stageTimeoutLimit = 180
stageConnected = False


pageWidth = 600#400
pageHeight = 600

xQuarterShift = 10
#constants for columns of UI
#pageXCenter = pageWidth/2
#pageXoneThird = pageWidth/3
#pageXtwoThird = pageWidth - pageXoneThird
#colX1_4 =pageWidth/5 + xQuarterShift
#colX2_4 =2*pageWidth/5 + xQuarterShift
#colX3_4 = 3*pageWidth/5 + xQuarterShift
#colX4_4 = 4* pageWidth/5 + xQuarterShift





pageXCenter = pageWidth/2
colX1_4 = 90
colX2_4 = 170
colX3_4 = 250
colX4_4 = 330


#Dictionaries for defining stage serial numbers, limits, and storing
# stage references (reference contains connection info for that stage)
# "rotCtrlSN" is the serial number for the 3-channel benchtop controller
# connected to the rotary stages
# Serial numbers should to be updated to match stage layout in QA lab
bar_x_SN = "45000001"
bar_y_SN = "45000002"
pd_x_SN = "45000003"
pd_y_SN = "45000004"
rot_ctrl_SN = "70000002"
pd_gpib = "27"

stageList = ['bar_x',  #x-axis linear stage that holds bar
             'bar_y',   #y-axis linear stage that holds bar
             'pd_x',    #x-axis linear stage that hold DAQ photodiode
             'pd_y',    #y-axis linear stage that hold DAQ photodiode
             'pd_rot',  #rotary stage that holds DAQ photodiode
             'laser_rot']#rotary stage that holds mirror for routing laser to bar


#rotCtrl - benchtop controller for the two rotary stages
# Because rotary stages are controlled using a benchtop controller instead of
# a direct connection to an integrated controller (like the linear stages),
# their serial numbers are not relevant for connecting to that stage

stageProps = {'bar_x'   :[bar_x_SN, 'linear', [0,150], ''],
             'bar_y'    :[bar_y_SN, 'linear', [0,150], ''],
             'pd_x'     :[pd_x_SN, 'linear', [0,150], ''],
             'pd_y'     :[pd_y_SN, 'linear', [0,150], ''],
             'pd_rot'   :['',         'rotary', [0,360], ''],
             'laser_rot':['',         'rotary', [0,360], ''],
             'rotCtrl'  :[rot_ctrl_SN, 'rotary', [0,0],   '']}




stageSN = {'bar_x': stageProps['bar_x'][0],\
           'bar_y': stageProps['bar_y'][0],\
           'pd_x' : stageProps['pd_x'][0],\
           'pd_y' : stageProps['pd_y'][0],\
           'rotCtrl': stageProps['rotCtrl'][0]}


stageType = {'bar_x'    : stageProps['bar_x'][1],\
             'bar_y'    : stageProps['bar_y'][1],\
             'pd_x'     : stageProps['pd_x'][1],\
             'pd_y'     : stageProps['pd_y'][1],\
             'pd_rot'   : stageProps['pd_rot'][1],\
             'laser_rot': stageProps['laser_rot'][1]}

stageLimits = {'bar_x'    : stageProps['bar_x'][2],\
               'bar_y'    : stageProps['bar_y'][2],\
               'pd_x'     : stageProps['pd_x'][2],\
               'pd_y'     : stageProps['pd_y'][2],\
               'pd_rot'   : stageProps['pd_rot'][2],\
               'laser_rot': stageProps['laser_rot'][2]}


#Dictionary of stage references. Definition are defined once
# stage is connected to. References can be considered pointers
# to the motorized stage's communication bus address
stageRef = {'bar_x': stageProps['bar_x'][3],\
            'bar_y': stageProps['bar_y'][3],\
            'pd_x' : stageProps['pd_x'][3],\
            'pd_y' : stageProps['pd_y'][3],\
            'pd_rot': stageProps['pd_rot'][3],\
            'laser_rot': stageProps['laser_rot'][3],\
            'rotCtrl' : stageProps['rotCtrl'][3]}


def connect(stage,root,verbose=False):
    #global status
    # verbose is a debug setting that causes the stage references to be
    # printed out to further verify that each stage can be connected to
    if verbose:
        for p in stageRef:
            print(p, stageRef[p],sep=' ')
        print()

    DeviceManagerCLI.BuildDeviceList()  

    if stageType[stage] == 'linear':      
        #connect to linear stage with serial number defined by dictionary
        serial_no = stageSN[stage]
        device = LongTravelStage.CreateLongTravelStage(serial_no)
        device.Connect(serial_no)

        #check whether stage has been initialized, initializes if not
        if not device.IsSettingsInitialized():
            device.WaitForSettingsInitialized(5000)  # 5 second timeout
            assert device.IsSettingsInitialized() is True
        
        # Start polling (looking at data from stage) and enable
        device.StartPolling(250)
        time.sleep(0.25)
        device.EnableDevice()
        time.sleep(0.25)

        # Load any config needed by the controller
        motor_config = device.LoadMotorConfiguration(serial_no)
        stageRef[stage] = device
        print(f'"{stage}" connected.')
        #status['text'] = f'"{stage}" connected.'
        #msg_box['text'] = f'"{stage}" connected.'

    elif stageType[stage] == 'rotary':
        #connect to rotary stage controller and then ch 1 and 2 for
        # actual stages
        serial_no = stageSN['rotCtrl']
        if stageRef['rotCtrl'] == '':
            device = BenchtopStepperMotor.CreateBenchtopStepperMotor(serial_no)
            device.Connect(serial_no)
            stageRef['rotCtrl'] = device
        else:
            device = stageRef['rotCtrl']
        time.sleep(0.25)
        chDict = {'pd_rot':2, 'laser_rot':1}
        ch = chDict[stage]

        # For benchtop devices, get the channel
        channel = device.GetChannel(ch)
        # Ensure that the device settings have been initialized
        if not channel.IsSettingsInitialized():
            channel.WaitForSettingsInitialized(5000)  # 5 second timeout
            assert channel.IsSettingsInitialized() is True

        # Start polling and enable
        channel.StartPolling(250)  # 250ms polling rate
        time.sleep(0.25)
        channel.EnableDevice()
        time.sleep(0.25)  # Wait for device to enable

        # Load any configuration settings needed by the controller/stage
        channel_config = channel.LoadMotorConfiguration(channel.DeviceID)
        chan_settings = channel.MotorDeviceSettings
        channel.GetSettings(chan_settings)
        channel_config.DeviceSettingsName = 'HDR50'
        channel_config.UpdateCurrentConfiguration()
        channel.SetSettings(chan_settings, True, False)
        
        stageRef[stage]= channel
        print(f'"{stage}" connected.')
        #status['text'] = f'"{stage}" connected.'

    else:
        print('ERROR: Stage type not recognized')
    root.update()
    root.update_idletasks()
    return
    #end connect()

def connectAll(root,stageList=['bar_x','bar_y','pd_x','pd_y','pd_rot','laser_rot']):
    #global status
    print('Connecting all stages:')
    #status['text'] = 'Connecting all stages...'
    for stage in stageList:
        connect(stage,root)
    return
    #end connectAll()

def connectPD(root,gpib=pd_gpib):
    meter = Keithley6517B('GPIB::'+str(gpib))
    return meter

def disconnect(stage):
    try:
        if stageType[stage] == 'linear':
            stageRef[stage].StopPolling()
            stageRef[stage].Disconnect()
            stageRef[stage] = ''
            print(f'"{stage}" disconnected.')
        elif stageType[stage] == 'rotary':
            stageRef[stage].StopPolling()
            stageRef[stage] == ''
            if ((stageRef['pd_rot'] == '') and (stageRef['laser_rot'] == '')):
                stageRef['rotCtrl'].Disconnect()
            print(f'"{stage}" disconnected.')
        else:
            print('ERROR: Stage type not recognized')
    except:
        print('error')
    return    
    #end disconnect()
   
def disconnectAll(stageList=['bar_x','bar_y','pd_x','pd_y','pd_rot','laser_rot']):
    global run
    global connected
    global root
    if connected:
        print('\nDisconnecting all stages:')
        root.update()
        for stage in stageList:
            disconnect(stage)
            root.update()
    run = False
    time.sleep(1)
    return
    #end disconnectAll()


def Home(stageName,root):
    stage = stageRef[stageName]
    if stage.get_NeedsHoming():
        print(f'Homing "{stageName}" ',end='')
        stage.Home(120000)  # 120 second timeout
        print(". Done.")
    else:
        print(f'"{stageName}" already homed.')
    root.update()
    root.update_idletasks()
    return
    #end Home()

def HomeAll(root,stageList=['bar_x','bar_y','pd_x','pd_y','pd_rot','laser_rot']):
    if tk.messagebox.askyesno(title='Home All Stages?', message='Home all stages?\n\nIf yes, all stages will\nbe reinitialized and\nmoved to 0 posiiton.'):
        print('Homing all stages:')
        for stageName in stageList:
            Home(stageName,root)
            root.update()
        print()
    return
    #end HomeAll()



def move(stage):
    global root
    global stageSet
    #get reference for stage name input to function
    ref = stageRef[stage]

    new_pos = float(stageSet[stage].get())

    #check that new_pos isn't an empty string (indicates that that
    # stage's input was skipped in command line interface
    old_pos = ref.get_Position()
    if new_pos == '':
        new_pos = old_pos
    else:
        new_pos = Decimal(float(new_pos))

    if stageProps[stage][1] == 'linear':
        LL = Decimal(stageProps[stage][2][0])
        UL = Decimal(stageProps[stage][2][1])
        if (new_pos < LL) or (new_pos > UL):
            new_pos = max(LL, min(new_pos, UL))
            print(f'Entered location out of\n\tstage limits.\n\tMoving to {new_pos}\n\tinstead.')
            stageSet[stage].delete(0,tk.END)
            stageSet[stage].insert(0,new_pos)
    elif stageProps[stage][1] == 'rotary':
        new_pos = new_pos % Decimal(360)
    else:
        print('ERROR: program is in an impossible state. Restart entire program.')
    
    # check that stage isn't already at input position, move to position if not
    if new_pos != old_pos:
        print(f'Moving "{stage}" from\n\t{old_pos} to {new_pos}',end='')
        root.update()
        ref.MoveTo(new_pos, 120000)  # 2min timeout    
        print(". Done.")
        root.update()
    else:
        print(f'"{stage}" already at {new_pos}.')
    return
    ###end move()

def nothing():
    pass
    return

def placeTextEntry(x,y,box_fill,text='',state='normal'):
    out = tk.Entry(root,width=10,state=state)
    out.insert(0,box_fill)
    out.place(x=x,y=y,anchor='n')
    if text != '':
        label = tk.Label(root,text=text,background='#c5c9c7')
        label.place(x=x-50,y=y,anchor='n')
    return out

def placeLabel(x,y,text,anchor='n'):
    label = tk.Label(root,text=text,background='#c5c9c7')
    label.place(x=x,y=y,anchor=anchor)
    return

def placeButton(x,y,text,command=nothing):
    out = tk.Button(root,text=text,command=command)
    out.place(x=x,y=y-4,anchor='n')
    return out

def placeRB(x,y,text='',width=9):
    out = tk.Label(root,width=width,background='gray87',text=text)
    out.place(x=x,y=y,anchor='n')
    return out

def log(items):
    global connected
    global fout
    out = ''
    
    if connected:
        if fout == '':
            fout = time.strftime(os.getcwd()+'\\'+'%Y-%m-%d_%H%M%S.dat', \
                      time.localtime())
            out += foutHeader
        for i in items[:-1]:
            out += i['text'] + '\t'
        out += str(items[-1].get()) +'\n'
        
        #print(out)
        with open(fout,'a') as f:
            f.write(out)
        
    return

def moveBar(barPos):
    global root
    if barPos.get() == 2:
        print('Moving bar in to beam',end='')
        root.update()
        stageRef['bar_x'].MoveTo(Decimal(barInPos[0]), 120000)
        stageRef['bar_y'].MoveTo(Decimal(barInPos[1]), 120000)
        stageRef['pd_y'].MoveTo(Decimal(barInPos[2]), 120000)
        stageRef['pd_y'].MoveTo(Decimal(barInPos[3]), 120000)
        stageRef['laser_rot'].MoveTo(Decimal(barInPos[3]), 120000)
        stageRef['pd_rot'].MoveTo(Decimal(barInPos[4]), 120000)
        print('.\n\tDone.')
    elif barPos.get() == 0:
        print('Moving bar out of beam',end='')
        root.update()
        stageRef['bar_x'].MoveTo(Decimal(barOutPos[0]), 120000)
        stageRef['bar_y'].MoveTo(Decimal(barOutPos[1]), 120000)
        stageRef['pd_y'].MoveTo(Decimal(barOutPos[2]), 120000)
        stageRef['pd_y'].MoveTo(Decimal(barOutPos[3]), 120000)
        stageRef['laser_rot'].MoveTo(Decimal(barOutPos[3]), 120000)
        stageRef['pd_rot'].MoveTo(Decimal(barOutPos[4]), 120000)
        print('.\n\tDone.')
    else:
        print("ERROR: program reached impossible state. Stop and restart everything")
    root.update()
    return   

def pdSkip():
    global skipPD
    global skipPD_Button
    if skipPD:
        skipPD_Button['text'] = 'Skip PDs'
        skipPD_Button['bg']= bkgColor
        skipPD = False
    else:
        skipPD_Button['text'] = 'PDs skipped'
        skipPD_Button['bg']= 'red'
        skipPD = True
    return


if __name__ == '__main__':

    class OutputRedirector:
        def __init__(self, text_widget):
            self.text_widget = text_widget

        def write(self, string):
            self.text_widget.insert(tk.END, string)
            self.text_widget.see(tk.END)  # Auto-scroll to the bottom

        def flush(self):
            pass
        
    root = tk.Tk()
    root.title("EIC DIRC QA Lab DAQ")
    root.configure(background='#c5c9c7')
    root.geometry(str(pageWidth)+'x'+str(pageHeight))
    canvas = tk.Canvas(root,width=pageWidth,height=pageHeight,background='#c5c9c7')
    canvas.pack()

    #Title label for UI
    title = tk.Label(root,text="EIC DIRC QA Lab DAQ",background='#c5c9c7',font='Helvetica 13 bold')
    title.place(x=pageXCenter,y=40,anchor='center')


    placeLabel(465, 75, 'Status')
    status= tk.Text(root,width=27,height=30,background='gray87')
    status.place(x=365,y=100,anchor='nw')
    redirector = OutputRedirector(status)
    sys.stdout = redirector


    #linear stages
    y = 70
    placeLabel(20,y,'Linear Stages','sw')
    canvas.create_rectangle(20,y,colX4_4+30,220,fill=bkgColor)
    
    y += 5
    placeLabel(colX1_4, y, 'Stage SN')
    placeLabel(colX2_4, y, 'Pos. Read')
    placeLabel(colX3_4+20, y, 'Pos. Set')
    placeLabel(colX4_4, y, 'Move')
    

    y += 25
    bar_x_SN_box = placeTextEntry(colX1_4,y,bar_x_SN,'Bar X')
    bar_x_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    bar_x_set = placeTextEntry(colX3_4+20,y,'')
    bar_x_move = placeButton(colX4_4,y,'Move',lambda: move('bar_x'))

    y += 30
    bar_y_SN_box = placeTextEntry(colX1_4,y,bar_y_SN,'Bar Y')
    bar_y_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    bar_y_set = placeTextEntry(colX3_4+20,y,'')
    bar_y_move = placeButton(colX4_4,y,'Move',lambda: move('bar_y'))

    y += 30
    pd_x_SN_box = placeTextEntry(colX1_4,y,pd_x_SN,'PD X')
    pd_x_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    pd_x_set = placeTextEntry(colX3_4+20,y,'')
    pd_x_move = placeButton(colX4_4,y,'Move',lambda: move('pd_x'))

    y += 30
    pd_y_SN_box = placeTextEntry(colX1_4,y,pd_y_SN,'PD Y')
    pd_y_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    pd_y_set = placeTextEntry(colX3_4+20,y,'')
    pd_y_move = placeButton(colX4_4,y,'Move',lambda: move('pd_y'))

    
    #rotary stage controller
    y = 245
    placeLabel(20,y,'Rotary Stages','sw')
    canvas.create_rectangle(20,y,colX4_4+30,370,fill=bkgColor)

    y += 5
    placeLabel(colX1_4, y, 'Stage SN')
    placeLabel(colX2_4, y, 'Pos. Read')
    placeLabel(colX3_4+20, y, 'Pos. Set')
    placeLabel(colX4_4, y, 'Move')
    
    y += 25 
    rot_ctrl_SN_box = placeTextEntry(colX1_4,y,rot_ctrl_SN,'Ctrlr')
    
    
    y += 30
    laser_rot_SN_box = placeTextEntry(colX1_4,y,'N/A','Laser')
    laser_rot_SN_box['state'] = 'disable'
    laser_rot_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+40,y,rotUnits)
    laser_rot_set = placeTextEntry(colX3_4+20,y,'')
    laser_rot_move = placeButton(colX4_4,y,'Move',lambda: move('laser_rot'))

    y += 30
    pd_rot_SN_box = placeTextEntry(colX1_4,y,'N/A','PD')
    pd_rot_SN_box['state'] = 'disable'
    pd_rot_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+40,y,rotUnits)
    pd_rot_set = placeTextEntry(colX3_4+20,y,'')
    pd_rot_move = placeButton(colX4_4,y,'Move',lambda: move('pd_rot'))

    
    homeAll = placeButton(colX4_4-20,395,'Home All Stages',lambda: HomeAll(root))


    #Photodiode UI

    y = 395
    placeLabel(20,y,'Photodiodes','sw')
    canvas.create_rectangle(20,y,colX2_4+60,550,fill=bkgColor)


    y += 5
    placeLabel(colX1_4, y, 'Addr.')
    placeLabel(colX2_4, y+30, 'Value Read')

    skipPD = False
    skipPD_Button = tk.Button(root,text='Skip PDs',width=10,command=pdSkip)
    skipPD_Button.place(x=colX2_4+60,y=y-4,anchor='ne')
    
    y += 25
    pd_gpib_box = placeTextEntry(colX1_4,y,pd_gpib,'Ctrlr')

    y += 30
    pd1_SN_box = placeTextEntry(colX1_4,y,'N/A','PD 1')
    pd1_SN_box['state'] = 'disable'
    pd1_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+45,y,pdUnits)

    y += 30
    pd2_SN_box = placeTextEntry(colX1_4,y,'N/A','PD 2')
    pd2_SN_box['state'] = 'disable'
    pd2_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+45,y,pdUnits)

    y += 30
    pd3_SN_box = placeTextEntry(colX1_4,y,'N/A','PD 2')
    pd3_SN_box['state'] = 'disable'
    pd3_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+45,y,pdUnits)


    # bar position
    y = 430    
    placeLabel(colX4_4-20,y,'Bar Position')

    barIn = tk.IntVar()
    pos = (('Bar Out',0),('Bar In    ',2))

    y += 25
    for p in pos:
        barPos = ttk.Radiobutton(root,text=p[0],value=p[1],variable=barIn)
        barPos.place(x=colX4_4-20,y=y,anchor='n')
        y += 25

    bar_move = placeButton(colX4_4-20,y+10,'Move Bar',lambda: moveBar(barIn))

    
    stageRB = {'bar_x': bar_x_RB,\
           'bar_y': bar_y_RB,\
           'pd_x' : pd_x_RB,\
           'pd_y' : pd_y_RB,\
           'pd_rot': pd_rot_RB,\
           'laser_rot': laser_rot_RB}


    enable_on_connect = [bar_x_set,bar_x_move,bar_y_set,bar_y_move,pd_x_set,\
              pd_x_move,pd_y_set,pd_y_move,laser_rot_set,laser_rot_move,\
              pd_rot_set,pd_rot_move,bar_move,skipPD_Button,homeAll]

    disable_on_connect = [bar_x_SN_box,bar_y_SN_box,pd_x_SN_box,pd_y_SN_box,\
                          rot_ctrl_SN_box,pd_gpib_box]

    for i in enable_on_connect:
        i['state'] = 'disable'


    stageRB = {'bar_x': bar_x_RB,\
                'bar_y': bar_y_RB,\
                'pd_x' : pd_x_RB,\
                'pd_y' : pd_y_RB,\
                'pd_rot': pd_rot_RB,\
                'laser_rot': laser_rot_RB}
            
    stageSet = {'bar_x': bar_x_set,\
                'bar_y': bar_y_set,\
                'pd_x' : pd_x_set,\
                'pd_y' : pd_y_set,\
                'pd_rot': pd_rot_set,\
                'laser_rot': laser_rot_set}

    
    #Handles using window's 'X' button to close UI
    root.protocol('WM_DELETE_WINDOW',disconnectAll)

    
    def ConnectHome():
        global root
        global connected
        global connButt
        global meter
        global SIMULATE
        connButt['text'] = 'Connecting...'
        connButt.config(relief='sunken')

        root.update_idletasks()
        root.update()
        if not connected:
            try:
                connectAll(root)
                print()
                #HomeAll(root)
                #print()
                if not SIMULATION:
                    meter = connectPD(root)
            except:
                pass
                connected = False
            else:
                for i in enable_on_connect:
                    i['state'] = 'normal'
                for i in disable_on_connect:
                    i['state'] = 'disabled'
                connected = True
                connButt['text'] = 'Connected'
                connButt['bg']= 'green'
        else:
            print('Already connected to system.')
        return

    y = 12
    timestamp = placeRB(colX1_4-30,y,width=15)
    connButt = placeButton(550,y,'Connect All',ConnectHome)
    connected = False

    #log button
    logList = [timestamp,bar_x_RB,bar_y_RB,pd_x_RB,pd_y_RB,laser_rot_RB,\
               pd_rot_RB,pd1_RB,pd2_RB,pd3_RB,barIn]

    
    logButton = tk.Button(root,text='LOG',width=40,command=lambda: log(logList))
    logButton.place(x=200,y=570,anchor='center')

    #Main loop - program just loops over this section of code when running
    while run:
        if SIMULATION: SimulationManager.Instance.InitializeSimulations()
        time.sleep(0.25)
        timestamp['text'] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        if connected:
            for stage in stageList:
                ref = stageRef[stage]
                pos = ref.get_Position()
                stageRB[stage]['text'] = pos

            if not SIMULATION:
                for i,pd in enumerate([pd1_RB,pd2_RB,pd3_RB],start=1):
                    if not skipPD:
                        meter.write(':ROUT:OPEN:ALL')
                        time.sleep(0.25)
                        meter.write(':ROUT:CLOS '+str(i))
                        pd['text'] = meter.current
                    else:
                        pd['text'] = 'skipped'
            else:        
                for i,pd in enumerate([pd1_RB,pd2_RB,pd3_RB],start=1):
                    if not skipPD:
                        pd['text'] = time.strftime("%S", time.localtime())
                    else:
                        pd['text'] = 'skipped'
            
            
            ##############################################################
            '''
            TO DO:
            
            Refine PD connect control and readback so it runs faster

            add move sequencing
                Idea: use a text file to feed commands for sequencing and DAQ

            
            '''
            ##############################################################

        
        root.update_idletasks()
        root.update()

    # uncomment line below if using Thorlabs Kinesis simulation mode
    if SIMULATION: SimulationManager.Instance.UninitializeSimulations()
    
    print('\nProgram closed.')
    root.destroy()
