#!/bin/bash

set -exo pipefail

COLOR_GREEN="\033[0;32m"
COLOR_OFF="\033[0m"

# clear log files
rm -rf experiment-*/
rm -rf experiment_*

start_2=$(date '+%Y_%m_%d/%H_%M_%S')
echo -e "${COLOR_GREEN}  [2] Katran-mqtt_fwd LB experiment: ${COLOR_OFF} Is .env ready ?"
sed -i 's/^SHARED_SUBS=.*/SHARED_SUBS=0/' .env      # ensure SHARED_SUBS is 0 for 1st and 2nd experiments
sed -i 's/^ONE_REAL_ONLY=.*/ONE_REAL_ONLY=0/' .env  # Set ONE_REAL_ONLY to 0 for 2nd experiment
export CONFIG_AND_OPERATE_LB=1 && export $(grep -v '^#' .env | xargs) && ./experiment.sh
mv experiment_logs/ experiment-LB/
end_2=$(date '+%d/%m/%Y_%H:%M:%S')

echo -e "####################  Katran-mqtt_fwd LB experiment results:  ####################\n\n" > all_results.txt
echo -e "Experiment 2 (Katran-mqtt_fwd LB) start time: $start_2, end time: $end_2\n" >> all_results.txt
cat experiment-LB/results.txt >> all_results.txt
echo -e "\n\n" >> all_results.txt

# 
# 
# docker exec -it client_0 sh   # repeat for 1,2,3,4,5
# export PAUSE=0 && sed -i 's/^ip.*eth0/#/' setup.sh && sed -i 's/^#\spython3/python3/' setup.sh  &&  ./setup.sh
# 
# docker compose stop $(docker compose ps --services | grep 'client_')