ace.define("ace/theme/msphysics_dark",["require","exports","module","ace/lib/dom"], function(e, t, n) {

t.isDark = false;
t.cssClass = "ace-msphysics_dark";
t.cssText = "\
.ace-msphysics_dark .ace_gutter {\
background-color: #111;\
color: #777;\
border-right: 1px solid #444;\
}\
.ace-msphysics_dark { 								/* GLOBAL bg and color */\
background-color: #151515;\
color: #FFF;\
}\
.ace-msphysics_dark .ace_variable { 			/* @ sign */\
color:#A8C;\
}\
.ace-msphysics_dark .ace_paren {				/*     () {}     */\
color: #FFF;\
}\
.ace-msphysics_dark .ace_cursor {\
color: #CCC;\
}\
.ace-msphysics_dark .ace_constant { 			/* true, false, nil, ... */\
color:#6BF;\
}\
.ace-msphysics_dark .ace_numeric {\
color:#5B5;\
}\
.ace-msphysics_dark .ace_keyword { 			/* if, elsif, else, ... + body events*/\
color:#F64;\
}\
.ace-msphysics_dark .ace_mp_keywords { 	/* MSPhysics methods*/\
color:#DA5;\
}\
.ace-msphysics_dark .ace_support {\
color:#CA9;\
}\
.ace-msphysics_dark .ace_operator {\
color: #FFF;\
}\
.ace-msphysics_dark .ace_function {\
color:#F33;\
}\
.ace-msphysics_dark .ace_string {\
color:#BD6;\
}\
.ace-msphysics_dark .ace_comment {\
color: #777;\
}\
.ace-msphysics_dark .ace_marker-layer .ace_selection {\
background-color: #456;\
border-radius: 0;\
}\
.ace-msphysics_dark .ace_marker-layer .ace_selected-word {\
border: 1px solid #BF0;\
box-sizing: border-box;\
background-color: #252;\
}\
.ace-msphysics_dark .ace_bracket {\
margin: -1px 0 0 -1px;\
border: 1px solid #DDD;\
}\
.ace-msphysics_dark .ace_marker-layer .ace_step {\
background-color: rgb(198, 219, 174);\
}\
.ace-msphysics_dark .ace_marker-layer .ace_active-line {\
background-color: rgba(100, 100, 100, 0.2);\
}\
.ace-msphysics_dark .ace_gutter-active-line {\
background-color: rgba(100, 100, 100, 0.2);\
}\
.ace-msphysics_dark .ace_invisible {\
color: #BFBFBF\
}\
.ace-msphysics_dark .ace_storage {\
color:#A52A2A;\
}\
.ace-msphysics_dark .ace_invalid.ace_illegal {\
color:#FD1224;\
background-color:rgba(255, 6, 0, 0.15);\
}\
.ace-msphysics_dark .ace_invalid.ace_deprecated {\
text-decoration:underline;\
font-style:italic;\
color:#FD1732;\
background-color:#E8E9E8;\
}\
.ace-msphysics_dark .ace_regexp{\
color:#FFF;\
background-color: rgba(100,50,50,0.5);\
}\
.ace-msphysics_dark .ace_meta.ace_tag {\
color:#005273;\
}\
.ace-msphysics_dark .ace_markup.ace_heading {\
color:#B8012D;\
background-color:rgba(191, 97, 51, 0.051);\
}\
.ace-msphysics_dark .ace_markup.ace_list{\
color:#8F5B26;\
}\
.ace-msphysics_dark .ace_print-margin {\
width: 1px;\
background-color: rgba(0,0,0,0);\
}\
.ace-msphysics_dark .ace_indent-guide {\
background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAACCAYAAACZgbYnAAAAEklEQVQImWMQERFpYLC1tf0PAAgOAnPnhxyiAAAAAElFTkSuQmCC) right repeat-y\
}";

var r = e("../lib/dom");
r.importCssString(t.cssText, t.cssClass);
});
