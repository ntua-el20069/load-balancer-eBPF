# Make several requests to see load balancing in action
for i in {1..10}; do
  curl http://localhost:1111/
  echo
done
