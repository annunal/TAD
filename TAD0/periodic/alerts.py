import datetime
import time
import os,sys
import readConfig as rc

today=datetime.datetime.utcnow()
config=[]
 
#print 'idDevice=',idDevice

def sendSMS(config, level, alert_signal):
  if config['SMSlist']=='':
    return
  template_SMS=config['SMSTemplate']
  if os.path.exists(template_SMS):
    f=open(template_SMS,'r')
    testo=f.read()
    f.close
 
    testo=testo.replace('$TITLE',config['title'])
    
    testo=testo.replace('$DATE',today.strftime("%d %b %Y"))
    testo=testo.replace('$TIME',today.strftime("%H:%M"))
    testo=testo.replace('$LEV',"{0:.2f}".format(level))
    testo=testo.replace('$ALERT_SIGNAL',format(alert_signal))
    testo=testo.replace('$IDdevice',idDevice)
    
    URL=config['SMSURL'].replace('$SMSUSER',config['SMSuser']).replace('$SMSPWD',config['SMSpwd'])
    URL=URL.replace('$SMSLIST',config['SMSlist'])
    URL=URL.replace('$MSG',testo)
    URL=URL.replace('$EQ','=')
    print URL
    os.system('wget --output-document=outcmd.txt "'+URL+'"')
    os.system('rm outcmd.txt')
  else:
    print '*** ERR:  Template file for periodic SMS does not exists:', template_SMS
    
def sendEMAIL(config, level, alert_signal):
  if config['EmailTo']=='':
    return
  body_file=config['EmailTemplate']
  subj_file=config['EmailSubject']
 
  if os.path.exists(body_file) and os.path.exists(subj_file):
    f=open(body_file,'r')
    testo=f.read()
    f.close
  
    f=open(subj_file,'r')
    subj=f.read()
    f.close
  
    subj=subj.replace('$TITLE',config['title'])
    subj=subj.replace('$IDdevice',idDevice)
    subj=subj.replace('$ALERT_SIGNAL',format(alert_signal))
    
    testo=testo.replace('$TITLE',config['title'])
    testo=testo.replace('$IDdevice',idDevice)
    testo=testo.replace('$DATE',today.strftime("%d %b %Y"))
    testo=testo.replace('$TIME',today.strftime("%H:%M"))
    testo=testo.replace('$LEV',"{0:.2f}".format(level))
    testo=testo.replace('$ALERT_SIGNAL',format(alert_signal))
    
    URL=config['EmailURL']
    URL=URL.replace('$TO',config['EmailTo'])
    URL=URL.replace('$SUBJ',subj)
    URL=URL.replace('$CONTENT',testo)
    URL=URL.replace('$EQ','=')
    print URL
    os.system('wget --output-document=outcmd.txt "'+URL+'"')
    os.system('rm outcmd.txt')
  else:
    print '*** ERR:  Template file (body or subject)for periodic EMAIL does not exists:', subj_file, body_file
    


newDateFile='newDateAlert.txt'

# devo farmelo dare in argomento
args=sys.argv

if len(args)>1:
  level=args[1]
  alert_signal=args[2]
else:
  level=0.1
  alert_signal=2  

#print 'Level=',level

#print 'EmailURL=',config['EmailURL']

#print '---------------- start of config-----------------\n\n'
#for i in config:
#       print i,'=',config[i]
#print '---------------- end of config-----------------\n\n' 
#---------------------------------------------------------


if os.path.exists(newDateFile):
  f=open(newDateFile,'r')
  newDate_s=f.read()
  f.close()

else: 
  config=rc.readConfig() 
  newDate_s='1 Jan 2000 00:00'

newDate=datetime.datetime.strptime(newDate_s,'%d %b %Y %H:%M')

#print newDate, 'UTC'

if today>newDate:
  config=rc.readConfig() 
  
  idDevice=config['IDdevice'].replace('$HOSTNAME',os.uname()[1])
  print 'sending sms and email'
  if alert_signal>=int(config['AlertLevel']):
    sendSMS(config,level, alert_signal)
    sendEMAIL(config,level, alert_signal)
    f=open(newDateFile,'w')
    deltaMin=int(config['AlertTimeInterval'])
  
    newDate=today+datetime.timedelta(minutes=deltaMin)
    newDate_s=newDate.strftime("%d %b %Y %H:%M")
    f.write(newDate_s)
    f.close()

  
print 'New alert after '+newDate_s +' UTC'
