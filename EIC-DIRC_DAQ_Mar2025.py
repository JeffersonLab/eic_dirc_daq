import tkinter as tk
from tkinter import ttk,messagebox
#from tkinter import messagebox
from datetime import datetime
from pymeasure.instruments.keithley import Keithley6517B
import time,os,socket,pymeasure,sys,clr

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



#changable settings for the program conveniently all in one place
bar_x_SN =      "45294954"
bar_y_SN =      "45299074"
pd_x_SN =       "45373284"
pd_y_SN =       "45375164"
rot_ctrl_SN =   "70373994"
pd_gpib =       "27"

stageList = ['bar_x',    #x-axis linear stage that holds bar
             'bar_y',    #y-axis linear stage that holds bar
             'pd_x',     #x-axis linear stage that hold DAQ photodiode
             'pd_y',     #y-axis linear stage that hold DAQ photodiode
             'pd_rot',   #rotary stage that holds DAQ photodiode
             'brew_rot']#rotary stage that holds mirror for routing laser to bar


#bar??Pos format = [bar_x, bar_y, pd_x, pd_y, laser_rot, pd_rot]
barInPos = [75,75,75,75,45,45]
barOutPos = [0,0,0,0,0,0]

dXDefault = 5
nColDefault = 3
dYDefault = 5
nRowDefault = 3


pdUnits = 'μA'
linUnits = 'mm'
rotUnits = '°'


if pdUnits == 'pA':
    pdUnitsFactor = 10**12
elif pdUnits == 'nA':
    pdUnitsFactor = 10**9
elif pdUnits == 'μA':
    pdUnitsFactor = 10**6
elif pdUnits == 'mA':
    pdUnitsFactor = 10**3
elif pdUnits == 'A':
    pdUnitsFactor = 1
else:
    pdUnitsFactor = 10**6
    
#Line below forces program to run with simulated devices when run from DSG's
#   development PC as opposed to trying to connect to non-existant read devices
SIMULATION = socket.gethostname() == 'DSGCONTROLS2'

if SIMULATION:
    bar_x_SN =      "45000001"
    bar_y_SN =      "45000002"
    pd_x_SN =       "45000003"
    pd_y_SN =       "45000004"
    rot_ctrl_SN =   "70000002"
    pd_gpib =       "27"    


# constants for log file formatting
fout = ''
foutHeader = 'timestamp\tbar_x\tbar_y\tpd_x\tpd_y\tlaser_rot\tpd_rot\tpd1\tpd2\tpd3\tbar in(2)/out(0)\n'

run = True
comConnect = False
stageTimeoutLimit = 180
stageConnected = False


#Constants used for GUI creation
bkgColor = '#c5c9c7'

pageWidth = 600
pageHeight = 600

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


#rotCtrl is the benchtop controller for the two rotary stages
#   Because rotary stages are controlled using a benchtop controller instead of
#   a direct connection to an integrated controller (like the linear stages),
#   their serial numbers are not relevant for connecting to that stage, hence
#   why those serial numbers are empty strings in dictionary

stageProps = {stageList[0]  :[bar_x_SN,     'linear', [0,300], ''],
              stageList[1]  :[bar_y_SN,     'linear', [0,150], ''],
              stageList[2]  :[pd_x_SN,      'linear', [0,300], ''],
              stageList[3]  :[pd_y_SN,      'linear', [0,150], ''],
              stageList[4]  :['',           'rotary', [0,360], ''],
              stageList[5]  :['',           'rotary', [0,360], ''],
              'rotCtrl'     :[rot_ctrl_SN,  'rotary', [0,0],   '']}


def connect(stage,root):

    DeviceManagerCLI.BuildDeviceList()  

    if stageProps[stage][1] == 'linear':      
        #connect to linear stage with serial number defined by dictionary
        serial_no = stageProps[stage][0]
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
        stageProps[stage][3] = device
        print(f'"{stage}" connected.')

    elif stageProps[stage][1] == 'rotary':
        #connect to rotary stage controller and then ch 1 and 2 for
        # actual stages
        serial_no = stageProps['rotCtrl'][0]
        if stageProps['rotCtrl'][3] == '':
            device = BenchtopStepperMotor.CreateBenchtopStepperMotor(serial_no)
            device.Connect(serial_no)
            stageProps['rotCtrl'][3] = device
        else:
            device = stageProps['rotCtrl'][3]
        time.sleep(0.25)
        chDict = {stageList[4]:2, stageList[5]:1}
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
        stageProps[stage][3]= channel
        print(f'"{stage}" connected.')
        
    else:
        print('ERROR: Stage type not recognized')
    root.update()
    root.update_idletasks()
    return
    #end connect()

def connectAll(root,stageList=stageList):
    print('Connecting all stages:')
    for stage in stageList:
        connect(stage,root)
    return
    #end connectAll()

def connectPD(root,gpib=pd_gpib):
    meter = Keithley6517B('GPIB::'+str(gpib))
    return meter

def disconnect(stage):
    try:
        if stageProps[stage][1] == 'linear':
            stageProps[stage][3].StopPolling()
            stageProps[stage][3].Disconnect()
            stageProps[stage][3] = ''
            print(f'"{stage}" disconnected.')
        elif stageProps[stage][1] == 'rotary':
            stageProps[stage][3].StopPolling()
            stageProps[stage][3] == ''
            if ((stageProps[stageList[4]][3] == '') and (stageProps[stageList[5]][3] == '')):
                stageProps['rotCtrl'][3].Disconnect()
            print(f'"{stage}" disconnected.')
        else:
            print('ERROR: Stage type not recognized')
    except:
        print('error')
    return    
    #end disconnect()
   
def disconnectAll(stageList=stageList):
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
    stage = stageProps[stageName][3]
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

def HomeAll(root,stageList=stageList):
    if tk.messagebox.askyesno(title='Home All Stages?', message='Home all stages?\n\nIf yes, all stages will\nbe reinitialized and\nmoved to 0 posiiton.'):
        print('Homing all stages:')
        for stageName in stageList:
            Home(stageName,root)
            root.update()
        print()
    return
    #end HomeAll()



def move(stage,new_pos = -9999):
    global root
    global stageSet
    #get reference for stage name input to function
    ref = stageProps[stage][3]

    if new_pos == -9999:
        new_pos = stageSet[stage].get()
    

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

def placeTextEntry(x,y,box_fill,text='',state='normal',width=10):
    out = tk.Entry(root,width=width,state=state)
    out.insert(0,box_fill)
    labelShift = 50
    if width != 10:
        x = x - 20
        labelShift = labelShift - 20
    out.place(x=x,y=y,anchor='n')
    if text != '':
        label = tk.Label(root,text=text,background='#c5c9c7')
        label.place(x=x-labelShift,y=y,anchor='n')
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
        if tk.messagebox.askyesno(title='Move Bar In to Beam?', message='Move bar in to beam?\n\nIf yes, all stages will\nmove to corresponding\npredefined posiiton.'):
            print('Moving bar in to beam',end='')
            root.update()
            stageProps[stageList[0]][3].MoveTo(Decimal(barInPos[0]), 120000)
            stageProps[stageList[1]][3].MoveTo(Decimal(barInPos[1]), 120000)
            stageProps[stageList[2]][3].MoveTo(Decimal(barInPos[2]), 120000)
            stageProps[stageList[3]][3].MoveTo(Decimal(barInPos[3]), 120000)
            stageProps[stageList[5]][3].MoveTo(Decimal(barInPos[3]), 120000)
            stageProps[stageList[4]][3].MoveTo(Decimal(barInPos[4]), 120000)
            print('.\n\tDone.')
    elif barPos.get() == 0:
        if tk.messagebox.askyesno(title='Move Bar Out of Beam?', message='Move bar out of beam?\n\nIf yes, all stages will\nmove to corresponding\npredefined posiiton.'):
            print('Moving bar out of beam',end='')
            root.update()
            stageProps[stageList[0]][3].MoveTo(Decimal(barOutPos[0]), 120000)
            stageProps[stageList[1]][3].MoveTo(Decimal(barOutPos[1]), 120000)
            stageProps[stageList[2]][3].MoveTo(Decimal(barOutPos[2]), 120000)
            stageProps[stageList[3]][3].MoveTo(Decimal(barOutPos[3]), 120000)
            stageProps[stageList[5]][3].MoveTo(Decimal(barOutPos[3]), 120000)
            stageProps[stageList[4]][3].MoveTo(Decimal(barOutPos[4]), 120000)
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

def moveSeqSetUp():
    global dX,dY,nCol,nRow,cmdQueue

    NROW = int(nRow.get())
    NCOL = int(nCol.get())
    DX = float(dX.get())
    DY = float(dY.get())
    
    cmdQueue = ['goToStart']
    for y in range(NROW):
        if y%2 == 0:
            direction = '+'
        else:
            direction = '-'
        for x in range(NCOL):
            cmdQueue.append('daq')
            if x != NCOL-1:
                cmdQueue.append('delta:bar_x:'+direction+str(DX))
        if y != NROW-1:
            cmdQueue.append('delta:bar_y:+'+str(DY))
    cmdQueue.append('goToStart')
    cmdQueue.append('done')
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


    placeLabel(370, 70, 'Status','sw')
    status= tk.Text(root,width=27,height=18,background='gray87')
    status.place(x=370,y=73,anchor='nw')
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
    bar_x_move = placeButton(colX4_4,y,'Move',lambda: move(stageList[0]))

    y += 30
    bar_y_SN_box = placeTextEntry(colX1_4,y,bar_y_SN,'Bar Y')
    bar_y_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    bar_y_set = placeTextEntry(colX3_4+20,y,'')
    bar_y_move = placeButton(colX4_4,y,'Move',lambda: move(stageList[1]))

    y += 30
    pd_x_SN_box = placeTextEntry(colX1_4,y,pd_x_SN,'PD X')
    pd_x_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    pd_x_set = placeTextEntry(colX3_4+20,y,'')
    pd_x_move = placeButton(colX4_4,y,'Move',lambda: move(stageList[2]))

    y += 30
    pd_y_SN_box = placeTextEntry(colX1_4,y,pd_y_SN,'PD Y')
    pd_y_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+50,y,linUnits)
    pd_y_set = placeTextEntry(colX3_4+20,y,'')
    pd_y_move = placeButton(colX4_4,y,'Move',lambda: move(stageList[3]))

    
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
    laser_rot_move = placeButton(colX4_4,y,'Move',lambda: move(stageList[5]))

    y += 30
    pd_rot_SN_box = placeTextEntry(colX1_4,y,'N/A','PD')
    pd_rot_SN_box['state'] = 'disable'
    pd_rot_RB = placeRB(colX2_4,y)
    placeLabel(colX2_4+40,y,rotUnits)
    pd_rot_set = placeTextEntry(colX3_4+20,y,'')
    pd_rot_move = placeButton(colX4_4,y,'Move',lambda: move(stageList[4]))

    


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

    homeAll = placeButton(colX4_4-30,395,'Home All Stages',lambda: HomeAll(root))
    
    y = 430
    xShift = 30
    placeLabel(colX4_4-xShift,y+10,'Bar Position')

    barIn = tk.IntVar()
    pos = (('Bar Out',0),('Bar In    ',2))

    y += 25
    for p in pos:
        barPos = ttk.Radiobutton(root,text=p[0],value=p[1],variable=barIn)
        barPos.place(x=colX4_4-xShift,y=y+10,anchor='n')
        y += 25

    bar_move = placeButton(colX4_4-xShift,y+20,'Move Bar',lambda: moveBar(barIn))


    #move sequencing
    x = 370
    y = 395
    placeLabel(x,y,'Move Sequencing','sw')
    canvas.create_rectangle(x,y,x+220,y+155,fill=bkgColor)

    x = 440
    y = 400
    xShift = 75
    dX = placeTextEntry(x,y,dXDefault,'Δx',width=5)
    dX['state'] = 'disable'
    placeLabel(x+10, y, 'mm')
    placeLabel(x+xShift, y, '# Cols.')
    nCol = placeTextEntry(x+xShift+60,y,nColDefault,'',width = 5)
    nCol['state'] = 'disable'
    
    y += 30
    dY = placeTextEntry(x,y,dYDefault,'Δy',width=5)
    dY['state'] = 'disable'
    placeLabel(x+10, y, 'mm')
    placeLabel(x+xShift, y, '# Rows')
    nRow = placeTextEntry(x+xShift+60,y,nRowDefault,'',width = 5)
    nRow['state'] = 'disable'

    y += 30
    moveSeqGo = placeButton(x+40,y+10,'Start Automated Move Sequence',moveSeqSetUp)



    enable_on_connect = [bar_x_set,bar_x_move,bar_y_set,bar_y_move,pd_x_set,\
              pd_x_move,pd_y_set,pd_y_move,laser_rot_set,laser_rot_move,\
              pd_rot_set,pd_rot_move,bar_move,skipPD_Button,homeAll,dX,\
              nCol,dY,nRow,moveSeqGo]

    disable_on_connect = [bar_x_SN_box,bar_y_SN_box,pd_x_SN_box,pd_y_SN_box,\
                          rot_ctrl_SN_box,pd_gpib_box]

    for i in enable_on_connect:
        i['state'] = 'disable'

    stageRB = {stageList[0]: bar_x_RB,\
           stageList[1] : bar_y_RB,\
           stageList[2] : pd_x_RB,\
           stageList[3] : pd_y_RB,\
           stageList[4] : pd_rot_RB,\
           stageList[5] : laser_rot_RB}  

    stageSet = {stageList[0]: bar_x_set,\
           stageList[1] : bar_y_set,\
           stageList[2] : pd_x_set,\
           stageList[3] : pd_y_set,\
           stageList[4] : pd_rot_set,\
           stageList[5] : laser_rot_set}
    
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

    
    logButton = tk.Button(root,text='LOG',width=80,command=lambda: log(logList))
    logButton.place(x=pageXCenter,y=575,anchor='center')


    cmdQueue = []
    
    #Main loop - program just loops over this section of code when running
    while run:
        if SIMULATION: SimulationManager.Instance.InitializeSimulations()
        time.sleep(0.25)
        timestamp['text'] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        if connected:
            for stage in stageList:
                ref = stageProps[stage][3]
                pos = ref.get_Position()
                stageRB[stage]['text'] = pos

            if not SIMULATION:
                skipPD = True   #############################################################################################
                for i,pd in enumerate([pd1_RB,pd2_RB,pd3_RB],start=1):
                    if not skipPD:
                        #meter.write(':ROUT:OPEN:ALL')
                        #time.sleep(0.25)
                        meter.write(':ROUT:CLOS (@ '+str(i)+')')
                        pd['text'] = round((meter.current) * pdUnitsFactor,4)
                    else:
                        pd['text'] = 'skipped'
            else:        
                for i,pd in enumerate([pd1_RB,pd2_RB,pd3_RB],start=1):
                    if not skipPD:
                        pd['text'] = time.strftime("%S", time.localtime())
                    else:
                        pd['text'] = 'skipped'
            

            if len(cmdQueue) > 0:
                cmdIn = cmdQueue[0]
                if cmdIn == 'goToStart':
                    move('bar_x',0)
                    move('bar_y',0)
                elif 'delta' in cmdIn:
                    cmd,stage,shift = cmdIn.strip().split(':')
                    new = float(stageRB[stage]['text']) + float(shift)
                    move(stage,new)
                elif cmdIn == 'daq':
                    print('Taking Measurement.')
                    log(logList)
                elif cmdIn == 'done':
                    print('\nMove sequence complete.')
                cmdQueue = cmdQueue[1:]

            
            ##############################################################
            '''
            TO DO:
            
            Refine PD connect control and readback so it runs faster

            Make so move sequence automatically starts a new log file

            
            '''
            ##############################################################

        
        root.update_idletasks()
        root.update()

    if SIMULATION: SimulationManager.Instance.UninitializeSimulations()
    
    print('\nProgram closed.')
    root.destroy()
