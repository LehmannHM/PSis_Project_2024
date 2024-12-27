import zmq
import score_update_pb2
import curses
import google.protobuf

def main(stdscr):
    context = zmq.Context()
    subscriber = context.socket(zmq.SUB)
    subscriber.connect("tcp://localhost:5557")  # Updated to new score update address
    subscriber.setsockopt_string(zmq.SUBSCRIBE, "")

    stdscr.clear()
    stdscr.addstr(0, 0, "Astronaut Scores")
    stdscr.refresh()

    while True:
        message = subscriber.recv()
        score_update = score_update_pb2.ScoreUpdate()
        try:
            score_update.ParseFromString(message)
        except message.DecodeError as e:
            stdscr.addstr(1, 0, f"Error parsing message: {e}")
            stdscr.refresh()
            continue

        stdscr.clear()
        stdscr.addstr(0, 0, "Astronaut Scores")
        for i, score in enumerate(score_update.scores):
            stdscr.addstr(i + 1, 0, f"Astronaut {chr(65 + i)}: {score}")
        stdscr.refresh()

if __name__ == "__main__":
    curses.wrapper(main)