#描述

用C实现的简易http服务器，以及一些测试html等文件

#用途

嵌入式环境，或者修改修改放到VPS上驱动简单的静态博客。

#配置

读取 /etc/nixhttpd.conf

例如写成：

	directory=/Users/nix/dev/c/nixhttpd_all/www_nix
	port=8888

配置网页所在目录和服务器端口


#健壮性没有保证

没有长期测试，就是这样。

#协议

GPL

