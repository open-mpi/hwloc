import subprocess, os

html_static_path = ['_build/html']
html_extra_path = ['./build/html']

read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'
if read_the_docs_build:
    subprocess.call('./readthedocs.io.sh ; doxygen readthedocs.io.cfg', shell=True)


