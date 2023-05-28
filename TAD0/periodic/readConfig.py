import io

def readConfig():
        # read the configuration file
        config={}
        fh=open('config.txt','r')
        righe=fh.readlines()

        for  riga in righe:
                if not riga.startswith('*'):
                        if '=' in riga:
                                tag=riga.split('=')[0].strip()
                                value=riga.split('=')[1].split('*')[0].strip()
                                #print tag,value
                                config[tag]=value

        fh.close()


        return config

