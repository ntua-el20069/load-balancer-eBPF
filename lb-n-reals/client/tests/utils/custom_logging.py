import logging
import logging.config
import sys

colors = {
    'magenta_color': "\033[35m",
    'bold_red_color': "\033[1;31m",
    'green_color': "\033[92m",
    'red_color': "\033[91m",
    'blue_color': "\033[94m",
    'yellow_color': "\033[93m",
    'reset_color': "\033[0m"
}

colors_map = {
    'DEBUG': colors['magenta_color'],
    'INFO': colors['green_color'],
    'WARNING': colors['yellow_color'],
    'ERROR': colors['red_color'],
    'CRITICAL': colors['bold_red_color'],
}

class ColoredFormatter(logging.Formatter):
    """A custom formatter that applies ANSI color codes to the log level name."""
    
    global colors, colors_map
    # Define the structure of the log message 
    DEFAULT_FORMAT = '%(asctime)s - %(name)s - [%(levelname)s] - %(message)s'

    def __init__(self, fmt=DEFAULT_FORMAT, datefmt=None, style='%'):
        # Pass the format string, date format, and style to the base class constructor
        super().__init__(fmt, datefmt, style)
        self.fmt = fmt # Store the format string for dynamic use

    def format(self, record):
        # 1. Get the color for the current log level
        log_level_name = record.levelname
        color = colors_map.get(log_level_name, colors["reset_color"])
        
        # 2. Temporarily modify the record's levelname to include color codes
        # This is the standard trick to use a custom formatter for color
        record.levelname = f'{color}{log_level_name:<8}{colors["reset_color"]}'
        
        # 3. Call the base class's format method to perform the substitutions 
        # using the format string provided during initialization
        formatted_message = super().format(record)
        
        # 4. Restore the original levelname, although not strictly necessary 
        # after the base format call completes.
        record.levelname = log_level_name 
        
        return formatted_message

def configure_colored_logging(level: str,
    module_name: str = __name__,
    path_to_custom_logger_file = "utils.custom_logging",
    include_time = False) -> logging.Logger:

    # 1. Define your colors and the custom Formatter class
    # --- ANSI Color Definitions ---
    # assert level in ['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'], 
    # "Invalid log level"
    global colors, colors_map
    assert level in colors_map.keys(), "Invalid log level (expected level in colors_map.keys())"

    # 2. Define the Configuration Dictionary
    LOGGING_CONFIG = {
        'version': 1,
        'disable_existing_loggers': False, # Allow existing loggers to be modified

        'formatters': {
            'colored': {
                # Use the full module path to your class. 
                # If the class is defined in the main script (__main__), use '__main__.ClassName'
                '()': f'{path_to_custom_logger_file}.ColoredFormatter', 
                'format': '%(asctime)s - %(name)s - [%(levelname)s] - %(message)s' if include_time else '%(name)s - [%(levelname)s] - %(message)s',
                'datefmt': '%Y-%m-%d %H:%M:%S'
            },
        },

        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'formatter': 'colored', # Use the formatter we defined above
                'level': level,
                'stream': sys.stdout, # Direct output to standard output
            }
        },

        'loggers': {
            # This is the root logger
            '': {
                'handlers': ['console'],
                'level': level,
                'propagate': True,
            },
            # You can define specific loggers if needed
            'my_module': { 
                'handlers': ['console'],
                'level': level,
                'propagate': False,
            }
            
        }
    }

    # 3. Load the configuration
    logging.config.dictConfig(LOGGING_CONFIG)

    # 4. Get the logger and test
    specific_log = logging.getLogger(name=module_name) # Use a specific logger if needed
    return specific_log

def main():
    """
    use this section to test the logging configuration independently.

    $ tree
    --- utils
        --- custom_logging.py  # this file
    --- test_logging.py  # a separate file to test logging

    In test_logging.py:

    from utils.custom_logging import configure_colored_logging
    log = configure_colored_logging(level="INFO", module_name=__name__, omit_time = True)
    log.debug("This is a debug message")
    log.info("This is an info message")
    log.warning("This is a warning message")
    log.error("This is an error message")
    """
    pass

if __name__ == "__main__":
    # print the docstring instructions
    print(main.__doc__)