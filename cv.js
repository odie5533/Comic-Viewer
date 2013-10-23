$(document).ready(function() {
	$("#download_button").fadeTo(0, 0.7).hover(function(){
		$(this).fadeTo("fast", 1);
	}, function(){
		$(this).fadeTo("slow", 0.7);
	});
	$("#navigation>a").hover(function(){
			$(this).find("img.active").fadeTo("fast", 1);
	}, function() {
			$(this).find("img.active").fadeTo("fast",0);
		}
	);
	Shadowbox.init({
		viewportPadding: 50
	});
});