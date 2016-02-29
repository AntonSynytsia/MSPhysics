@echo off
cd /d %~dp0
cls
yardoc '../MSPhysics.rb' '*.rb' --no-private - 'LICENCE.txt' 'FAQ.md' 'Overview.md' 'Examples.md' 'TODO.md'
rem yard graph --full -f graph.dot
