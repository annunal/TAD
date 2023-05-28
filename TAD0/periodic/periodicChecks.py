import datetime
import time
import os,sys
import readConfig as rc

today=datetime.datetime.utcnow()
config=[]
 
#print 'idDevice=',idDevice

def sendPeriodicSMS(config, level):
  template_SMS=config['TemplatePeriodic_SMS_Msg']
  if os.path.exists(template_SMS):
    f=open(template_SMS,'r')
    testo=f.read()
    f.close
 
    testo=testo.replace('$TITLE',config['title'])
    
    testo=testo.replace('$DATE',today.strftime("%d %b %Y"))
    testo=testo.replace('$TIME',today.strftime("%H:%M"))
    testo=testo.replace('$LEV',"{0:.2f}".format(level))
    testo=testo.replace('$IDdevice',idDevice)
    
    URL=config['SMSURL'].replace('$SMSUSER',config['SMSuser']).replace('$SMSPWD',config['SMSpwd'])
    URL=URL.replace('$SMSLIST',config['SMSlistPeriodic'])
    URL=URL.replace('$MSG',testo)
    URL=URL.replace('$EQ','=')
    print URL
    os.system('wget --output-document=outcmd.txt "'+URL+'"')
    os.system('rm outcmd.txt')
  else:
    print '*** ERR:  Template file for periodic SMS does not exists:', template_SMS
    
def sendPeriodicEMAIL(config, level):
  body_file=config['TemplatePeriodic_EMAIL_Body']
  subj_file=config['TemplatePeriodic_EMAIL_Subj']
 
  if os.path.exists(body_file) and os.path.exists(subj_file):
    f=open(body_file,'r')
    testo=f.read()
    f.close
  
    f=open(subj_file,'r')
    subj=f.read()
    f.close
  
    subj=subj.replace('$TITLE',config['title'])
    subj=subj.replace('$IDdevice',idDevice)
    
    testo=testo.replace('$TITLE',config['title'])
    testo=testo.replace('$IDdevice',idDevice)
    testo=testo.replace('$DATE',today.strftime("%d %b %Y"))
    testo=testo.replace('$TIME',today.strftime("%H:%M"))
    testo=testo.replace('$LEV',"{0:.2f}".format(level))
    
    URL=config['EmailURL']
    URL=URL.replace('$TO',config['EmailToPeriodic'])
    URL=URL.replace('$SUBJ',subj)
    URL=URL.replace('$CONTENT',testo)
    URL=URL.replace('$EQ','=')
    print URL
    os.system('wget --output-document=outcmd.txt "'+URL+'"')
    os.system('rm outcmd.txt')
  else:
    print '*** ERR:  Template file (body or subject)for periodic EMAIL does not exists:', subj_file, body_file
    


newDateFile='newDatePeriodic.txt'

# devo farmelo dare in argomento
args=sys.argv

if len(args)>1:
  level=float(args[1])
else:
  level=0.1  

#print 'Level=',level

#print 'EmailURL=',config['EmailURL']

#print '---------------- start of config-----------------\n\n'
#for i in config:
#       print i,'=',config[i]
#print '---------------- end of config-----------------\n\n' 
#---------------------------------------------------------


if os.path.exists(newDateFile):
  f=open(newDateFile,'r')
  newDate_s=f.read().replace('\n','')
  f.close()

else: 
  config=rc.readConfig() 
  #newDate_s='1 Jan 2000 00:00'
  delta=0
  Hour=config['Periodic_hour']
  newDate=today+datetime.timedelta(days=delta)
  newDate_s=today.strftime("%d %b %Y ")+Hour
  newDate=datetime.datetime.strptime(newDate_s,'%d %b %Y %H:%M')
  print 'newDate =',newDate 
  if newDate<today: 
	  delta=1
	  newDate=today+datetime.timedelta(days=delta)
	  newDate_s=newDate.strftime("%d %b %Y ")+Hour
	  print newDate_s
	  datetime.datetime.strptime(newDate_s,'%d %b %Y %H:%M')

  f=open(newDateFile,'w')
  f.write(newDate_s)
  f.close()

newDate=datetime.datetime.strptime(newDate_s,'%d %b %Y %H:%M')

print newDate, 'UTC'

if today>newDate:
  config=rc.readConfig() 
  
  idDevice=config['IDdevice'].replace('$HOSTNAME',os.uname()[1])
  print 'sending sms and email'
  sendPeriodicSMS(config,level)
  sendPeriodicEMAIL(config,level)
  f=open(newDateFile,'w')
  try:
	delta=int(config['Periodic_delta'])
  except:
	delta=1
  Hour=config['Periodic_hour']
  newDate=today+datetime.timedelta(days=delta)
  newDate_s=newDate.strftime("%d %b %Y ")+Hour
  f.write(newDate_s)
  f.close()

  
print 'New check '+newDate_s +' UTC'
