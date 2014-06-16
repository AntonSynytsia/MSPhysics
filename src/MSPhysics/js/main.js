String.prototype.contains = function(it) { return this.indexOf(it) != -1; };

function isInt(n) {
   return typeof n === 'number' && parseFloat(n) == parseInt(n, 10) && !isNaN(n);
}

function callback(name, data){
	if (!data) data = '';
	window.location.href = 'skp:' + name + '@' + data;
}

$(document).on('click', 'a[href]', function(){
	if (this.href.contains("http")){
		callback('open_link', this.href);
		return false;
	}
});

function init(){
	var w = $( window ).width();
	var h = $( window ).height();
	var data = '['+w+','+h+']';
	callback('init', data);
}

function editorSetScript(code){
	var editor = ace.edit('editor-container');
	editor.getSession().setValue(code);
}

function editorGoToLine(line){
	var editor = ace.edit('editor-container');
	editor.gotoLine(line);
}

function editorSelectCurrentLine(){
	var editor = ace.edit('editor-container');
	editor.selection.selectLineEnd();
}

function activateTab(number) {
	$('#tab'+number).fadeIn(800).siblings().hide();
	$('a[href$="#tab'+number+'"]').parent('li').addClass('active').siblings().removeClass('active');
}


$(document).ready( function(){

	// Initialize
	window.setTimeout(init, 0);

	var audioElement = document.createElement('audio');
	var audioLinks = [
		'https://googledrive.com/host/0B3qg8f4WrNdHWVVGVFp5Umt5Mk0/Never%20Gonna%20Say%20Im%20Sorry.mp3',
		'https://googledrive.com/host/0B3qg8f4WrNdHWVVGVFp5Umt5Mk0/Unspeakable.mp3',
		'https://googledrive.com/host/0B3qg8f4WrNdHWVVGVFp5Umt5Mk0/Change%20with%20the%20light.mp3',
		'https://googledrive.com/host/0B3qg8f4WrNdHWVVGVFp5Umt5Mk0/One%20Day.mp3',
		'https://googledrive.com/host/0B3qg8f4WrNdHWVVGVFp5Umt5Mk0/Rixton%20-%20Me%20and%20My%20Broken%20Heart.mp3'
	];
	var index = 0;

	// Initialize Ace editor
	var container = document.getElementById("editor-container");
	var saved = false;
	var editor = ace.edit("editor-container");
	editor.setTheme("ace/theme/TextMate");
	editor.session.setMode("ace/mode/ruby");
	editor.setSelectionStyle("text");
	editor.setHighlightActiveLine(true);
	editor.setShowInvisibles(false);
	editor.setDisplayIndentGuides(true);
	editor.setAnimatedScroll(true);
	editor.renderer.setShowGutter(true);
	editor.renderer.setShowPrintMargin(true);
	editor.session.setUseSoftTabs(true);
	editor.session.setTabSize(4);
	editor.setHighlightSelectedWord(true);
	editor.setBehavioursEnabled(true);
	editor.setFadeFoldWidgets(true);
	editor.setOption("scrollPastEnd", false);
	editor.on("change", function(e) {
		var code = editor.getSession().getValue();
		mword = code.toLowerCase();
		if (mword == "help\n" || mword == "help!\n")
			editor.getSession().setValue('Click on the links above to start off ;)');
		if (mword == "hello\n" || mword == "hello!\n" || mword == "hello world\n"){
			editor.getSession().setValue("Hello there, feeling lonely? Turn up the volume :D");
			audioElement.setAttribute('src', audioLinks[index]);
			if (index < audioLinks.length-1)
				index += 1;
			else
				index = 0;
			audioElement.setAttribute('autoplay', 'autoplay');
			audioElement.Play();
		}
		saved = false;
	});

	function editorSaveScript() {
		if (saved) return;
		var code = editor.getSession().getValue();
		$( '#temp-area' ).val(code);
		var pos = editor.selection.getSelectionAnchor();
		callback('editor_changed', pos.row+1);
		saved = true;
	}

	// Save editor script when the area looses focus.
	$('#editor-container').focusout(function() {
		editorSaveScript();
	});
	$('#editor-container').mouseout(function() {
		editorSaveScript();
	});
	// Focus editor on mouse enter
	$('#editor-container').mouseenter(function() {
		editor.focus();
	});

	// Initialize tabs
	$('.tabs .tab-links a').on('click', function(e){
		var currentAttrValue = $(this).attr('href');
		// Show/Hide Tabs
		$('.tabs ' + currentAttrValue).fadeIn(800).siblings().hide();
		// Change/remove current tab to active
		$(this).parent('li').addClass('active').siblings().removeClass('active');
		callback('tab_changed', currentAttrValue);
		e.preventDefault();
	});

	// Resize editor
	$( window ).resize( function(){
		container.style.height = $( window ).height() - 107 + "px";
		editor.resize();
	});
	$( window ).resize();

	// Validate numeric input.
	/*$('.numeric-input').keypress(function(evt) {
		var key = evt.keyCode || evt.which;
		key = String.fromCharCode( key );
		var regex = /[0-9]|\.|\-/;
		if( !regex.test(key) ) {
			evt.returnValue = false;
			if(evt.preventDefault) evt.preventDefault();
		}
	});*/
	$('.numeric-input').attr("maxlength", 64);
	$('.numeric-input').focusout(function() {
		try {
			with (Math) {
				var num = eval(this.value);
			}
			if (typeof num != 'number')
				throw 0;
		}
		catch(err) {
			var num = 0;
		}
		this.value = num.toFixed(2);
	});

	// Randomize body background
	/*if (Math.floor(Math.random() * 2) == 1){
		$( 'body' ).css("background", "rgb(180,130,190)");
		$( 'body' ).css("background", "-moz-linear-gradient(top,  rgba(180,130,190,1) 0%, rgba(182,150,188,1) 100%)");
		$( 'body' ).css("background", "-webkit-gradient(linear, left top, left bottom, color-stop(0%,rgba(180,130,190,1)), color-stop(100%,rgba(182,150,188,1)))");
		$( 'body' ).css("background", "-webkit-linear-gradient(top,	 rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
		$( 'body' ).css("background", "-o-linear-gradient(top,	 rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
		$( 'body' ).css("background", "-ms-linear-gradient(top,	 rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
		$( 'body' ).css("background", "linear-gradient(to bottom,	rgba(180,130,190,1) 0%,rgba(182,150,188,1) 100%)");
		$( 'body' ).css("filter", "progid:DXImageTransform.Microsoft.gradient( startColorstr='#b482be', endColorstr='#b696bc',GradientType=0 )");
	}*/
});
