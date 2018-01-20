#!/usr/bin/python
import cgi, cgitb
cgitb.enable()
import os, sys
import glob
import cgi
import cgitb; cgitb.enable()
import os, sys
import Cookie
import time

handler = {}

if 'HTTP_COOKIE' in os.environ:
	cookies = os.environ['HTTP_COOKIE']
	cookies = cookies.split(';')
	for cook in cookies:
		cook = cook.split('=')
		handler[cook[0]] = cook[1]
crole = ""		
isL = ""
for k in handler:
    if k == " ROLE":
	crole = handler[k]
    if k == " USER":
	if handler[k] is not None:
		isL = handler[k]



UPLOAD_DIR = "./Pictures"
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

TUPLOAD_DIR = "./Titles"
if not os.path.exists(TUPLOAD_DIR):
    os.makedirs(TUPLOAD_DIR)

HTML_TEMPLATE1 = """
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <title>Edit Picture Title</title>
</head>
<style>
div {
border: 2px solid black;
padding-bottom: 10px;
width: 350px; 
margin:0 auto;
}
p {
  text-align: center;
}
</style>
  <body><div>
    <h1 align="center">Delete Picture</h1> """
HTML = """
    <form align="center" action="delete.cgi"
	  method="POST" enctype="multipart/form-data">
    <input type="submit" name ="newtitle" value="Delete Picture">
   """

HTML_TEMPLATE2= """ 
    </form>
    <form align="center" action="gallery.cgi" method="POST">
<input value="Cancel" type="submit">
</form>
</div>
    <p id="filePath"></p>
    
    <script>
      function getFile() {
      var file = document.getElementById("file1");
      }
    </script>
  </body>
</html>
"""
HTML_RED = """
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="refresh" content="0;gallery.cgi">
</head>
<body>
</body>
</html> """

form = cgi.FieldStorage()
def removeImage(form, upload_dir, tupload_dir):
    if not form.has_key("hid"): return
    hiditem = form["hid"]
    path = hiditem.value + ".jpg"
    os.remove(os.path.join(upload_dir, path))
    path2 = hiditem.value + ".thumbnail"
    os.remove(os.path.join(upload_dir, path2)) 
    path3 = hiditem.value + ".txt"
    os.remove(os.path.join(tupload_dir,path3))
    print HTML_RED
    
    

if (isL == "") or (crole != "Owner"):
    print "Content-Type: text/html"
    print "Location:login.html\n"
elif not form.has_key("imgpath"):
    removeImage(form, UPLOAD_DIR, TUPLOAD_DIR)
else:
    dataHTML = '<p> Are you sure you would like to delete image: ' + form["imgpath"].value + '? </p>'
    print HTML_TEMPLATE1 + dataHTML + HTML + ' <input type="hidden" name="hid" value="' + form["imgpath"].value + '">' + HTML_TEMPLATE2

