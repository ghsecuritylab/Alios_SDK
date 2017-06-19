#!/bin/bash
TIME=/usr/bin/time
timestamp=`date +%F_%T`
INPUT=4175764634412383261319054216609912

if [[ $1 == -d ]] ; then
    DIR=$2
    shift;shift
else
    DIR=.
fi

if (( $# > 0 )) ; then 
    prefix=_${1}
else
    prefix=sparse
fi

#No QIH, runtime
for i in `seq 1 10`
do
  ${TIME} -o ${DIR}/cfrac_native_${timestamp}_${i}.time ../cfrac ${INPUT}
done

