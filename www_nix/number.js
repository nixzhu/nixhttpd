<script src="jquery.js" type="text/javascript"></script>
<script type="text/javascript">
	var finish = false;
	function read_number() {
		var all = 0;
		var a = 0;
		$.getJSON("test.json?t="+new Date().getTime(), function(data) {
			all = data.all;
			a = data.a;
			var html='';
			html +=  (data.a).toString()+ "_";
			html +=  (data.all).toString()+ "_";
			html += (data.a / data.all *100).toFixed("1") + "%";
			$("#newtext").html(html);
			$("#newtext").width(700*data.a / data.all);
			if (!finish && data.a == data.all) {
				
				$("h1").append("升级完毕!");
				finish = true;
			}
		});
		if (!finish) {
			setTimeout('read_number()', 1000);
		} else {
			alert("升级完毕!");
		}
			
	}
	$(document).ready(function () {
		read_number();
	});
</script>
