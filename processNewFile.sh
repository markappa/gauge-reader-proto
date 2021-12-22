#!/usr/bin/with-contenv bashio
# this script is called by inotifyd
# parameters are: event dirName fileName
# e.g.: "n     /media/espcam_02        20211212_042514.jpg"

event=$1
sourceDir=$2
lastFile=$3
pathName="$sourceDir/$lastFile"
archDir="$sourceDir/parsed"

CEN=$(bashio::config 'center')
RAD=$(bashio::config 'radius')
ANG=$(bashio::config 'angle')
VAL=$(bashio::config 'value')
THR=$(bashio::config 'threshold')

# test number of parameters
# test event is n or t (test, do not move file)
# test file is *.jpg
# test readGauge return value for error state
# test new value format x.xxx
# test curl return value for error state
# test curl http reply for OK200

function handleSigterm () {
  case $? in
    139) echo "segfault occurred" ;
      curl -s -X POST -H "Authorization: Bearer ${SUPERVISOR_TOKEN}"\
       -H "Content-Type: application/json"\
       -d "{\
        \"message\":\"readGauge failed with SIGTERM on file ${pathName}\",\
        \"logger\":\"${HOSTNAME}.addon\"\
       }"\
       http://supervisor/core/api/services/system_log/write ;;
    0) echo "all seems ok";;
  esac
}

echo "Processing file: $pathName"

trap handleSigterm EXIT

readCmd="/root/readGauge -c $CEN -r $RAD -A $ANG -V $VAL -t $THR $pathName"
#echo $readCmd

newVal=$($readCmd)
echo "Value read: $newVal"

if [ $event = "n" ]
then
curl -s -X POST -H "Authorization: Bearer ${SUPERVISOR_TOKEN}"\
 -H "Content-Type: application/json"\
 -d "{\"state\":$newVal,\
  \"attributes\": {\
   \"unit_of_measurement\":\"bar\",\
   \"state_class\":\"measurement\",\
   \"entity_category\":\"diagnostic\",\
   \"device_class\":\"pressure\",\
   \"unique_id\":\"xxiimarzo39mndmrc001\",\
   \"name\":\"Pressione caldaia\"\
  }\
 }"\
 http://supervisor/core/api/states/sensor.pressione_caldaia2
fi

echo

if [ $event = "n" ]
then
  mv $pathName $archDir
  echo "Moved $pathName to $archDir"
fi

exit

