#!/bin/bash

echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml/DTD/xhtml1-strict.dtd">'

echo '<html><head><title>File List</title>'
echo '<meta http-equiv="Content-Type" content="text/html; chartset=utf-8" />'

echo '<link href="style.css" rel="stylesheet" type="text/css"></head><body>'

echo '<ul id="menu">'
echo '<li><a href="index.html">主页</a></li>'
echo '<li><a href="upload.html">上传</a></li>'
echo '<li><a href="file_list.sh">文件查看</a></li>'
echo '</ul>'

echo '<h1>文件列表</h1>'
echo '<p>下面列出了已经上传至服务器的文件，包括时间信息和大小。</p>'

echo '<pre>'
ls -l ../www_nix/upload/
echo '</pre>'

echo '<div id="footer">Copyright &copy 2012 @nixzhu</div>'
echo '</body></html>'



