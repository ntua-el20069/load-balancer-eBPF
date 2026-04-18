# Monitor Past Experiments

Here we provide a docker `compose.yaml` that can be used in order to inspect experiments done in the past.

## Steps

### When you do the experiment

<details>
<summary> Skip these steps if you just want to re-inspect an already done test/experiment </summary>

- Save the date and time of the experiment (e.g. 25/02/2026  18:43 - 19:00)
- After doing the experiment, save a snapshot of the prometheus-volume database using this command: 
```bash
curl -X POST http://localhost:9091/api/v1/admin/tsdb/snapshot
# will return something like that
# {"status":"success","data":{"name":"20260323T195116Z-3362f5edae6bcc15"}}
```
- Then Go to Docker Desktop and follow this path: `Volumes > lb-n-reals_prometheus_data > data > snapshots`
- right click on the snapshot that has the corresponding name
- `Save as` and name that like `snapshot-*.zip` and place that inside `lb-n-reals/evolution/prometheus/data`

</details>

### Inspect the test measurements

Once you have the prometheus snapshot of the experiment in the `test/experiment`,
- `unzip` the desired snapshot zip file  and  docker compose the `past_monitor`
```bash
# prepare the prometheus data
cd lb-n-reals/evolution/prometheus/data
unzip <snapshot_file.zip>

cd ../../../../past_monitor/
docker compose up --build -d
```

- Launch `grafana_sample` container and login (admin/admin) 
- Go to Connections > Add new data source > Prometheus > URL: `http://prometheus_sample:9090` > Save & Test

- Go to Dashboards > New > Import > (e.g. `test_9.json`) > Select Prometheus as the data source > Import
- If you have saved the time of the experiment you can use `dashboard_with_xdp_metrics.json` and then Go to the Dashboard and select the time range the experiment was done, else if you just want to re-inspect one of the previously done experiments you can use `tests_dashboards/test_*.json`. (The dashboards differ only in the fields `time`, `title`,`uid`)  
- You are ready to inspect cpu / memory usage, network traffic of the containers during the experiment 


## Inspect Logs

- Go inside a `lb-n-reals/evolution/test_<...>` and check the average MQTT publish period (seconds) of the clients
```bash
grep -orP "Average time per message:\s+[0-9]+\.[0-9]+" 
```
- Inspect the `experiments_results.txt` to see how many publish messages from each client were successfully delivered to reals (and which reals received them)