#! /usr/bin/python

#===========================================================================
# SVN properties (DO NOT CHANGE)
#
# $Id$
# $HeadURL$
# $LastChangedRevision$
# $Author$
# $LastChangedDate$
#
#============================================================================

import os
import sys
import re
import datetime
import shutil
import subprocess
import getpass
from difxdb.business.versionhistoryaction import *
from optparse import OptionParser
from difxdb.difxdbconfig import DifxDbConfig
from difxdb.business.experimentaction import *
from difxdb.model.dbConnection import Schema, Connection
from string import lower, strip
from os.path import isdir
from subprocess import Popen, PIPE


__author__="Helge Rottmann <rottmann@mpifr-bonn.mpg.de>"
__prog__ = os.path.basename(__file__)
__build__= "$Revision$"
__date__ ="$Date$"
__lastAuthor__="$Author$"

minSchemaMajor = 1
minSchemaMinor = 1

def getUsage():
    
    usage = ""
    usage += "%s   %s  %s (last changes by %s) \n" % (__prog__, __build__, __author__, __lastAuthor__)
    usage += "A program to archive a correlated difx experiment.\n"
    usage += "Usage: %s <expcode> <path> \n\n"  % __prog__
    usage += "<expcode>:   the experiment code e.g. ey010a\n"
    usage += "<path>:  the path to the experiment directory\n"
    usage += "NOTE: %s requires the DIFXROOT environment to be defined." % __prog__
    usage += "The program reads the database configuration and other parameters from difxdb.ini located under $DIFXROOT/conf."

    
    return(usage)

def exitOnError(exception):
	'''
	Exit routine to be called whenever an error/exception has occured
	'''
	print "\nERROR: %s. Aborting\n\n" % exception
	
        # destroy kerberos tickets
        destroyTicket()
        
	exit(1)

def getTicket(user):
    
    print ("Obtaining kerberos ticket for user %s" % user)
    password = getpass.getpass("Enter password for user %s:" % (user))
    
    kinit = '/usr/bin/kinit'
    kinit_args = [ kinit, '%s' % (user) ]
    kinit = Popen(kinit_args, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    kinit.stdin.write('%s\n' % password)
    kinit.wait()


def destroyTicket():
    print ("Destroying kerberos ticket" )
    
    subprocess.call('/usr/bin/kdestroy')
    

def readConfig():
    
    
    
    if (os.getenv("DIFXROOT") == None):
            sys.exit("Error: DIFXROOT environment must be defined.")
            
    configName = os.getenv("DIFXROOT") + "/conf/difxdb.ini"
        
    config = DifxDbConfig(configName)
    
    if not config.sectionExists("difxarchive"):
        
        print ("Configuration file does not contain a section for difxarchive yet. Adding a default one for you. Please edit %s and restart difxarchive." % configName)
        config.addSection("difxarchive")
        config.set('difxarchive', 'archiveserver', 'ADD_ARCHIVE_SERVER')
        config.set('difxarchive', 'defaultuser', 'ADD_DEFAULT_USER')
        config.set('difxarchive', 'archiveremotepath', 'ADD_REMOTE_PATH')
        config.set('difxarchive', 'refbackuppath', 'ADD_PATH')
        
        config.writeConfig()
        exit(0)
    
    return (config)

def getTransferFileCount(path, user, config):
    
    server = config.get("difxarchive", "archiveserver")
    remotePath = config.get("difxarchive", "archiveremotepath")
   
    cmd = 'rsync -az --stats --dry-run %s %s@%s:%s' % ( path, user, server, remotePath) 
    proc = subprocess.Popen(cmd,
                                       shell=True,
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE,
                                       )
    remainder = proc.communicate()[0]
    
    
    mn = re.findall(r'Number of files transferred: (\d+)', remainder)
    fileCount = int(mn[0])
    
    print "Number of files to be transferred: %d " % fileCount
    
    return(fileCount)
    
def syncDir(path, user, config, fileCount, dryRun):
    
    
    server = config.get("difxarchive", "archiveserver")
    remotePath = config.get("difxarchive", "archiveremotepath")
    
    print "Copying files to: %s" % server
    
    if dryRun:
        cmd = 'rsync -av  --dry-run %s %s@%s:%s' % ( path, user, server, remotePath) 
    else:    
        cmd = 'rsync -av  --progress %s %s@%s:%s' % ( path, user, server, remotePath) 
        
    proc = subprocess.Popen(cmd,
                                       shell=True,
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE,
                                       )
    
    while True and not dryRun:
        output = proc.stdout.readline()

        if 'to-check' in output:
            
             m = re.findall(r'to-check=(\d+)/(\d+)', output)
             progress = (100 * (int(m[0][1]) - int(m[0][0]))) / fileCount
             
             sys.stdout.write('\rDone: ' + str(progress) + '%')
             sys.stdout.flush()
             
             if int(m[0][0]) == 0:
                      break
                      
    print('\rFinished')

        
    
    return

def syncReferenceDir(path, referencePath, dryRun):
    
    includePattern = ["*.vex", "*.skd", "*.v2d", "*.input", "*.difxlog", "*.log"]
    
    # check that destination path has trailing slash
    if not referencePath.endswith(os.path.sep):
        referencePath += os.path.sep
        
    # check that source path has NO trailing slash
    if  path.endswith(os.path.sep):
        path = path[:-1]
    
    cmd = "rsync -av --progress --include '*/' "  
    
    for pattern in includePattern:
        cmd += " --include '%s' " % pattern
    
    if dryRun:
        cmd += " --dry-run "
    
    cmd += " --exclude '*' --exclude '*.difx' %s %s" % ( path, referencePath) 

    print "Copying reference files to: %s" % referencePath
    
    proc = subprocess.Popen(cmd,
                                       shell=True,
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE,
                                       )
   
    
    print('\rFinished')
    return

def confirmAction():
    
     # if --force option was used skip confirmation
    if not options.force:
            
        print 'Are you sure you want to proceed? [y/N]'
        a = lower(sys.stdin.readline())
        if strip(a) == 'y':
            print 'OK -- proceeding\n'
        else:
            print 'Not continuing.\n'

            # destroy kerberos tickets
            destroyTicket()
            exit(0)
    
    
if __name__ == "__main__":

    usage = getUsage()
    
    parser = OptionParser(version="%prog " + __build__, usage=usage)
    
    parser.add_option("-u", "--user", dest="user", type="string" ,action="store", help="Do the archival as the specified user. This overrides the defaultuser directive in difxdb.ini")
    parser.add_option("-f", "--force", dest="force" ,action="store_true", default=False, help="Delete files without further confirmation ")
    parser.add_option("-d", "--dry-run", dest="dryRun" ,action="store_true", default=False, help="Simulate archival. Don't rsync files, don't update database. ")
    # parse the command line. Options will be stored in the options list. Leftover arguments will be stored in the args list
    (options, args) = parser.parse_args()   
     
    if len(args) < 2 :
	parser.print_help()
	exit(0)

    config = readConfig()

    # try to open the database connection
    connection = Connection(config)
    connection.echo = False
    
    if options.user is None:
       user = config.get("difxarchive", "defaultuser")
    else:
       user = options.user

    try:
        dbConn = Schema(connection)
        session = dbConn.session()
    except Exception as e:
        exitOnError(e)

    code = lower(args[0])
    path = args[1]

    # check that experiment exists in the database
    if not experimentExists(session, code):
        exitOnError("Experiment with code %s not found in the database." % code)
        
    # check if the experiment has already been archived
    if isExperimentArchived(session, code):
        print "Experiment has been archived already."
        confirmAction()
        
        
        
    # check that path exists
    if not isdir(path):
        exitOnError("Directory %s does not exist" % (path))

    if not isSchemaVersion(session, minSchemaMajor, minSchemaMinor):
        major, minor = getCurrentSchemaVersionNumber(session)
        exitOnError("Current difxdb database schema is %s.%s but %s.%s is minimum requirement." % (major, minor, minSchemaMajor, minSchemaMinor))
    
    try:
        # obtain kerberos ticket
        getTicket(user)

        while True:
            
            fileCount = getTransferFileCount(path, user, config)
            
            if (fileCount == 0):
                break
                
            # copy files to the archive server
            syncDir(path, user, config, fileCount, options.dryRun, )
            
            if options.dryRun:
                break
                

        # copy subset of files to the reference backup location
        syncReferenceDir(path, config.get("difxarchive", "refbackuppath"), options.dryRun)
        
        
        if not options.dryRun:
            # delete files
            print 'Archival process completed. Now deleting path %s including all files and subdirectories' % path
            confirmAction()

            shutil.rmtree(path)
            
            # update database
            experiment = getExperimentByCode(session, code)
            experiment.dateArchived = datetime.datetime.now()
            experiment.archivedBy = user
            session.commit()
            session.flush()


    except Exception as e:
        exitOnError(e)
    except KeyboardInterrupt:
        sys.exit(1)
    finally:
        # destroy kerberos tickets
        destroyTicket()
        
    
   