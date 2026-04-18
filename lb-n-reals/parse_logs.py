import re
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--reals", type=int, default=6)
    parser.add_argument("--log_dir", type=str, default=".")
    args = parser.parse_args()
    
    # client_id -> {real_num -> count}
    sent_publish_msgs = {}
    
    for real_num in range(0, 1 + args.reals):
        with open(f"{args.log_dir}/real_{real_num}.log", "r") as logfile:
            lines = logfile.readlines()

        for line in lines:
            regex_pattern = r"(PUBLISH|MESSAGE) from client_([0-9]+).*(measurements[^']+).*([0-9]+)\s+bytes"
            publish_msg = re.search(regex_pattern, line)
            if not publish_msg:
                continue
            
            client = int(publish_msg.group(2))
            topic = publish_msg.group(3)
            message_len = int(publish_msg.group(4))

            if client not in sent_publish_msgs:
                sent_publish_msgs[client] = {}
            if real_num not in sent_publish_msgs[client]:
                sent_publish_msgs[client][real_num] = 0
            sent_publish_msgs[client][real_num] += 1
        
    for client, reals in sent_publish_msgs.items():
        total_LB_messages = 0
        print(f"\n client_{client} sent to ", end="")
        for real_num, count in reals.items():
            print(f"    real_{real_num}: {count}", end="")
            # dont count publish messages to real_0 as LB, 
            # real_0 is used only in single broker experiment
            if real_num > 0:
                total_LB_messages += count
        print(f"   Total LB messages (sum): {total_LB_messages}", end="")
