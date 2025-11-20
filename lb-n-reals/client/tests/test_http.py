import unittest
import requests
import os
import re
from typing import Tuple
from utils.custom_logging import configure_colored_logging

log = configure_colored_logging(level="INFO", module_name=__name__)

TIMEOUT = 3

# concerning logging and printing, unittest captures stdout/stderr by default.
# pytest -s                  # disable all capturing

if os.getenv("LOCAL_TESTS") is not None:
    VIP_TO_ALL_LINK = f"http://127.0.0.1:8001/"
else:
    VIP_TO_ALL_LINK = f"http://{os.getenv('VIP_ALL')}:8000"

def parse_response(response: dict) -> Tuple[int, str]:
    message = response.get("message", "")
    pattern = r"from\s+server\s+(\d+)"
    match = re.search(pattern, message)
    if match:
        return (int(match.group(1)), message)
    else:
        raise ValueError("Message format is incorrect")

class KatranBasic(unittest.TestCase):
    log.info(f"\n\n {20*'-'} KatranBasic tests {20*'-'}")

    def setUp(self) -> None:
        log.debug("Setting up test case...")
        if os.getenv("LOCAL_TESTS") is not None:
            log.debug("Running local tests... omit assertion for VIP_ALL")
            return
        self.assertTrue(os.getenv("VIP_ALL") is not None, "Environment variable VIP_ALL must be set")

    def tearDown(self) -> None:
        log.debug("Tearing down test case...")
        pass
    
    def get_response(self, link: str, desired_status: int) -> requests.Response:
        try:
            response = requests.get(link, timeout=TIMEOUT)
            if response.status_code != desired_status:
                self.fail(f"Unexpected status code: {response.status_code} and response: {response.text}")
            return response
        except requests.exceptions.RequestException as e:
            self.fail(f"Request failed: {e}")
        except Exception as e:
            self.fail(f"An unexpected error occurred: {e}")

    def test_katran_basic(self):
        log.info(f"Test: {self._testMethodName}")
        response = self.get_response(VIP_TO_ALL_LINK, 200)
        server_num, message = parse_response(response.json())
        log.info(f"server_num: {server_num}, message: {message}")

    def test_weighted_katran(self):
        log.info(f"Test: {self._testMethodName}")
        server_hits = {}
        total_requests = 50

        for _ in range(total_requests):
            response = self.get_response(VIP_TO_ALL_LINK, 200)
            server_num, _ = parse_response(response.json())
            server_hits[server_num] = server_hits.get(server_num, 0) + 1

        for server, hits in server_hits.items():
            log.info(f"Server {server} handled {hits} requests out of {total_requests}. Percentage: {hits / total_requests * 100:.2f}%")
