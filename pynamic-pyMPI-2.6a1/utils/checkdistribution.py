import sys
import os
import string

def getCVSTree(directory):
    files = []
    try:
        f = open(os.path.join(directory,'CVS','Entries'))
    except IOError:
        pass
    else:
        for line in f.xreadlines():
            print 'try %r in %r'%(line,directory)
            try:
                actor = os.path.join(directory,line.split('/')[1])
            except:
                continue

            if line[:1] == '/':
                files.append(actor)
            elif line[:1] == 'D':
                files = files + getCVSTree(actor)
    return files


if __name__ == '__main__':
    # -----------------------------------------------
    # Name of the XXXXX.tar.gz file
    # -----------------------------------------------
    tgz = sys.argv[1]
    open(tgz) # Will verify existance

    # -----------------------------------------------
    # Name without the tar.gz extension
    # -----------------------------------------------
    name = string.join(tgz.split(os.extsep)[:-2],os.extsep)

    # -----------------------------------------------
    # Read out all the names (titles) in the file
    # -----------------------------------------------
    pipe = os.popen('gunzip -c %s | tar tf -'%tgz)
    tar_files = pipe.read().split()
    status = pipe.close()
    if status is not None: sys.exit(status)

    # -----------------------------------------------
    # Walk through the CVS tree and verify that each name
    # exists
    # -----------------------------------------------
    CVS_files = getCVSTree(os.curdir)
    CVS_files.sort()
    status = 0
    missing = []
    for x in CVS_files:
        lookup = name+x[len(os.curdir):]
        if lookup not in tar_files:
            missing.append(x[len(os.curdir)+len(os.sep):])
            status = 1

    # -----------------------------------------------
    # Merge the old list and the new list
    # -----------------------------------------------
    if status:
        extra_dist = sys.argv[2:]+missing
        extra_dist.sort()
        top_level = [x for x in extra_dist if os.sep not in x]
        sub_level = [x for x in extra_dist if os.sep in x]
        last_directory = None
        print 'EXTRA_DIST =',
        sorted = top_level + sub_level
        for x in sorted:
            directory,base = os.path.split(x)
            if x in  ['boot']: continue
            if directory in ['tests','experimental','obsolete']: continue

            if directory and directory == last_directory:
                tab = '\t\t'
            else:
                tab = '\t'
            last_directory = directory

            if x in missing:
                miss = '*'
            else:
                miss = ''
            sys.stdout.write(' \\\n%s%s%s'%(miss,tab,x))
        print


    sys.exit(status)

    
    
