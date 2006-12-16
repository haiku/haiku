#!/bin/bash

#cd po
for i in *.po
do 
	wget http://www.iro.umontreal.ca/translation/maint/wget/$i -O $i
done
