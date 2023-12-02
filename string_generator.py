import random
import string

def randomString(stringLength=10):
    """Generate a random string of fixed length """
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(stringLength))

string = randomString(21328000)
with open("mesage_earth.txt", "w") as f:
	f.write(string)

