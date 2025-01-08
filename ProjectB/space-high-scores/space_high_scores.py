import zmq
import score_update_pb2
import curses
import google.protobuf

def main(stdscr):
    context = zmq.Context()
    subscriber = context.socket(zmq.SUB)
    subscriber.connect("tcp://localhost:5557")
    subscriber.setsockopt_string(zmq.SUBSCRIBE, "")

    stdscr.clear()
    stdscr.addstr(0, 0, "Astronaut Scores")
    stdscr.refresh()

    # Dictionary to store the highest scores
    highest_scores = {chr(65 + i): 0 for i in range(8)}

    while True:
        message = subscriber.recv()
        score_update = score_update_pb2.ScoreUpdate()
        try:
            score_update.ParseFromString(message)
        except message.DecodeError as e:
            stdscr.addstr(1, 0, f"Error parsing message: {e}")
            stdscr.refresh()
            continue

        # Update the highest scores
        for i, score in enumerate(score_update.scores):
            astronaut = chr(65 + i)
            if score > highest_scores[astronaut]:
                highest_scores[astronaut] = score

        # Display the highest scores
        stdscr.clear()
        stdscr.addstr(0, 0, "Astronaut Scores")
        for i, (astronaut, score) in enumerate(highest_scores.items()):
            stdscr.addstr(i + 1, 0, f"Astronaut {astronaut}: {score}")
        stdscr.refresh()

if __name__ == "__main__":
    curses.wrapper(main)