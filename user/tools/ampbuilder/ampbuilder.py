#! /usr/bin/env python

import sys, os, subprocess

def main(argv): 

    if len(argv) != 4:
        sys.stderr.write("Usage: {0} <loader> <rsw0> <rsw1>\n".format(argv[0]))
        return 1
        
    for arg in argv[1:]:
        if not os.path.exists(arg):
            sys.stderr.write("File {0} not found\n".format(arg))
            return 1
    
    if not format(argv[1]).endswith('.o'):
        sys.stderr.write("The first parameter must be an object file\n")
        return 1
      
    loader, rsw0, rsw1 = argv[1:]  
    envvars = get_envvars()
    wrap_files(loader, rsw0, rsw1, envvars)
    return 0
    
    
def wrap_files(loader, rsw0, rsw1, envvars):
    filename = 'datos'
    imagename = 'image'
    create_datafile(filename + '.c', find_entrypoint(rsw0), find_entrypoint(rsw1))
    generate_binary(rsw0, 'rsw0', envvars)
    generate_binary(rsw1, 'rsw1', envvars)
    command = format(envvars['TARGET_OBJCOPY']) + ' --add-section .rsw0=rsw0 --set-section-flags .rsw0=\'alloc,load\''' ' + loader + ' ' + imagename + '.o'
    exec_command(command)
    command2 = format(envvars['TARGET_OBJCOPY']) + ' --add-section .rsw1=rsw1 --set-section-flags .rsw1=\'alloc,load\''' ' + imagename +'.o'
    exec_command(command2)
    generate_lds(imagename +'.lds', rsw0, rsw1, envvars)
    link_image(imagename + '.o', filename + '.c', imagename + '.lds', envvars)
    os.remove(imagename + '.o');
    os.remove(imagename + '.lds');
    os.remove(filename + '.c');
    os.remove(filename + '.o');
    os.remove('rsw0');
    os.remove('rsw1');
    
#def wrap_files(loader, rsw0, rsw1, envvars):
    #filename = 'datos.c'
    ##imagename = 'image'
    #create_datafile(filename, find_entrypoint(rsw0), find_entrypoint(rsw1))
    ##link_loader(loader, filename, envvars)
    #generate_binary(rsw0, 'rsw0', envvars)
    #generate_binary(rsw1, 'rsw1', envvars)
    #command = format(envvars['TARGET_OBJCOPY']) + ' --add-section .rsw0=rsw0 --set-section-flags .rsw0=\'alloc,load\''' ' + loader + ' image.o'
    #exec_command(command)
    #command2 = format(envvars['TARGET_OBJCOPY']) + ' --add-section .rsw1=rsw1 --set-section-flags .rsw1=\'alloc,load\''' image.o'
    #exec_command(command2)
    #generate_lds('image.lds', rsw0, rsw1, envvars)
    #link_image('image.o', filename, 'image.lds', envvars)
    #os.remove('image.o');
    #os.remove('image.lds');
    #os.remove('datos.c');
    #os.remove('datos.o');
    #os.remove('rsw0');
    #os.remove('rsw1');
    
def link_image(image, datafile, ldsfile, envvars): 
    datafilename = os.path.splitext(datafile)[0]
    command = format(envvars['TARGET_CC']) + ' -c ' + datafile + ' && ' + format(envvars['TARGET_LD']) + ' -o image ' + image + ' ' + datafilename + '.o -T ' + ldsfile 
    exec_command(command) 

def generate_lds(filename, rsw0, rsw1, envvars):
    f = open(filename, 'w')
    content = 'SECTIONS {\n\t. = 0x40000000;\n\t.text ALIGN(8) : {*(.text) }\n\t.data ALIGN(8) : {*(.data) }\n\t.bss ALIGN(8) : {*(.bss) }\n\t. = 0x' + find_startaddr(rsw0, envvars) + ';\n\t.rsw0 ALIGN(8) : {}\n\t. = 0x' + find_startaddr(rsw1, envvars) + ';\n\t.rsw1 ALIGN(8) : {}\n}'
    f.write(content)
    f.close() 
    
def generate_binary(filename, binfilename, envvars):
    command = format(envvars['TARGET_OBJCOPY']) + ' -O binary ' + filename + ' ' + binfilename
    exec_command(command)
                                            
def create_datafile(filename, epoint0, epoint1):
    f = open(filename, 'w')
    f.write('unsigned long ePoint0 = ' + epoint0 + ';\n')
    f.write('unsigned long ePoint1 = ' + epoint1 + ';\n')
    f.close()

def find_startaddr(filename, envvars):
    command = format(envvars['TARGET_OBJDUMP']) + ' -h ' + filename
    output = exec_command(command).split()
    startaddr = output.pop(output.index('.text') + 3)
    return startaddr

def find_entrypoint(filename):
    command = 'readelf -h ' + filename
    output = exec_command(command).split()
    epoint = output.pop(output.index('address:') + 1)
    return epoint

def exec_command(command): 
    execute = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    output = execute.communicate()[0]
    if execute.returncode:
        sys.exit(1)
    return output

def get_envvars():
    envvars = {}
    envvars['TARGET_CC'] = load_envvar('TARGET_CC')
    envvars['TARGET_LD'] = load_envvar('TARGET_LD')
    envvars['TARGET_OBJCOPY'] = load_envvar('TARGET_OBJCOPY')
    envvars['TARGET_OBJDUMP'] = load_envvar('TARGET_OBJDUMP')
    return envvars
    
def load_envvar(var):
    envvar = os.getenv(var)
    if envvar is None:
        print(var + ' environment variable not defined')
        sys.exit(1)
    return envvar       

if __name__ == '__main__':
    sys.exit(main(sys.argv))
    
