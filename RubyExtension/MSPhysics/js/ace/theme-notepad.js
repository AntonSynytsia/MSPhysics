ace.define("ace/theme/notepad",["require","exports","module","ace/lib/dom"], function(e, t, n) {

t.isDark = false;
t.cssClass = "ace-notepad";
t.cssText = "\
.ace-notepad .ace_gutter {\
background-color: #111;\
color: #777;\
border-right: 1px solid #444;\
}\
.ace-notepad { 								/* GLOBAL bg and color */\
background-color: #151515;\
color: #FFF;\
}\
.ace-notepad .ace_variable { 			/* @ sign */\
color:#48C;\
}\
.ace-notepad .ace_paren {				/*     () {}     */\
color: #FFF;\
}\
.ace-notepad .ace_cursor {\
color: #CCC;\
}\
.ace-notepad .ace_constant { 			/* true, false, nil, ... */\
color:#6BF;\
}\
.ace-notepad .ace_numeric {\
color:#FD4;\
}\
.ace-notepad .ace_keyword { 			/* if, elsif, else, ... + body events*/\
color:#9E7;\
font-weight: bold;\
}\
.ace-notepad .ace_mp_keywords { 	/* MSPhysics methods*/\
color:#DA5;\
}\
.ace-notepad .ace_support {\
color:#CA9;\
}\
.ace-notepad .ace_operator {\
color: #FFF;\
font-weight: normal;\
}\
.ace-notepad .ace_function {\
color:#F33;\
}\
.ace-notepad .ace_string {\
color:#D70;\
}\
.ace-notepad .ace_comment {\
color: #777;\
}\
.ace-notepad .ace_marker-layer .ace_selection {\
background-color: #555;\
border-radius: 0;\
}\
.ace-notepad .ace_marker-layer .ace_selected-word {\
border: 1px solid #BF0;\
box-sizing: border-box;\
background-color: #252;\
}\
.ace-notepad .ace_bracket {\
margin: -1px 0 0 -1px;\
border: 1px solid #DDD;\
}\
.ace-notepad .ace_marker-layer .ace_step {\
background-color: rgb(198, 219, 174);\
}\
.ace-notepad .ace_marker-layer .ace_active-line {\
background-color: rgba(100, 100, 100, 0.2);\
}\
.ace-notepad .ace_gutter-active-line {\
background-color: rgba(100, 100, 100, 0.2);\
}\
.ace-notepad .ace_invisible {\
color: #BFBFBF\
}\
.ace-notepad .ace_storage {\
color:#A52A2A;\
}\
.ace-notepad .ace_invalid.ace_illegal {\
color:#FD1224;\
background-color:rgba(255, 6, 0, 0.15);\
}\
.ace-notepad .ace_invalid.ace_deprecated {\
text-decoration:underline;\
font-style:italic;\
color:#FD1732;\
background-color:#E8E9E8;\
}\
.ace-notepad .ace_regexp{\
color:#FFF;\
background-color: rgba(100,50,50,0.5);\
}\
.ace-notepad .ace_meta.ace_tag {\
color:#005273;\
}\
.ace-notepad .ace_markup.ace_heading {\
color:#B8012D;\
background-color:rgba(191, 97, 51, 0.051);\
}\
.ace-notepad .ace_markup.ace_list{\
color:#8F5B26;\
}\
.ace-notepad .ace_print-margin {\
width: 1px;\
background-color: rgba(0,0,0,0);\
}\
.ace-notepad .ace_indent-guide {\
background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAACCAYAAACZgbYnAAAAEklEQVQImWMQERFpYLC1tf0PAAgOAnPnhxyiAAAAAElFTkSuQmCC) right repeat-y\
}";

var r = e("../lib/dom");
r.importCssString(t.cssText, t.cssClass);
});
