# enumeration converter
# Converts enumerations in this form (as defined in jetlib.h):
# /* enable Enumeration ******************************************************/
# #define HP_eOn                      0
# #define HP_eOff                     1
# to
# static AttrValue gEnableEnum[] = {
#	{HP_eOn, "eOn"}, 
#	{HP_eOff, "eOff"} 
#};

startFound=false
firstLine=false
while read line ; do
	start=$(echo "$line" | cut -d' ' -f 1)
	if [ "$start" = "/*" ] ; then
		if [ $startFound = true ] ; then
			echo
			echo \}\;
		fi
		startFound=true
		firstLine=true
		name=$(echo "$line" | cut -d' ' -f 2)
		echo
		echo static AttrValue g"$name"Enum[] = \{
	elif [ "$start" = "#define" ] ; then
		attr=$(echo "$line" | cut -d' ' -f 2)
		const=$(echo "$attr" | cut -b 4-)
		if [ $firstLine = false ] ; then
			echo ,
		else
			firstLine=false
		fi 
		echo -n $'\t' \{"$attr", \""$const"\"\}
	fi
done
if [ $startFound = true ] ; then
	echo
	echo \}\;
fi
