import cv2
import face_recognition
import serial
import tkinter as tk
from tkinter import simpledialog, messagebox
from PIL import Image, ImageTk

# Initialize serial communication
ser = serial.Serial('COM3', 9600)

# Trusted Faces List
knownFaces = [face_recognition.load_image_file("captured_face.jpg")]
knownNames = ["MWA"]  # A name for each trusted face
knownFacesEncodings = [face_recognition.face_encodings(image)[0] for image in knownFaces]  # Encodings for each face

# Ignored Faces List (Initialized with no faces)
ignoredFaces = []
ignoredFacesEncodings = []

# Counters
counter = 0  # Frame Counter
facesCounter = len(knownFaces)  # Faces Counter (Initialized with the number of known faces)
ignoredFacesCounter = 0  # Ignored Faces Counter
message = 0
lastMessage = 90

# Starting the camera feed
cap = cv2.VideoCapture(0)

# Function to handle auhorised
def handle_unauthorised(frame, top, right, bottom, left):
    global facesCounter, ignoredFacesCounter, knownFacesEncodings, ignoredFacesEncodings, lastMessage, message
    def action_open(): # Function to be called when opening for unknown person
            global facesCounter, ignoredFacesCounter, knownFacesEncodings, ignoredFacesEncodings, lastMessage, message
            ignoredFacesCounter += 1
            # Capture the face of the person
            face_roi = frame[top:bottom, left:right]
            # Save the image of the face
            cv2.imwrite(f"ignored_face{ignoredFacesCounter}.jpg", face_roi)
            print("Photo captured!")
            # Add the face to the ignored list
            ignoredFaces.append(face_recognition.load_image_file(f"ignored_face{ignoredFacesCounter}.jpg"))
            ignoredFacesEncodings = [face_recognition.face_encodings(image)[0] for image in ignoredFaces]
            print("door opening")
            message = 2
            if message != lastMessage: # Only send if the message is changed from the last state
                    ser.write(b"2")
                    lastMessage = 2
    
    def action_add(): # Function to be called when adding user
            global facesCounter, ignoredFacesCounter, knownFacesEncodings, ignoredFacesEncodings, lastMessage, message, counter
            nonlocal frame, top, bottom, left, right
            facesCounter += 1
            # Capture the face of the person and ask for his name
            newName = simpledialog.askstring("Input", "Enter the person's name:")
            face_roi = frame[top:bottom, left:right]
            # Save the image of the face
            cv2.imwrite(f"captured_face{facesCounter}.jpg", face_roi)
            print("Photo captured!")
            # Add the face to the trusted list
            knownFaces.append(face_recognition.load_image_file(f"captured_face{facesCounter}.jpg"))
            knownNames.append(newName)
            knownFacesEncodings = [face_recognition.face_encodings(image)[0] for image in knownFaces]
            counter += 1

    def action_ignore(): # Function to be called when ignoring a person
            global facesCounter, ignoredFacesCounter, knownFacesEncodings, ignoredFacesEncodings, lastMessage, message
            ignoredFacesCounter += 1
            # Capture the face of the person
            face_roi = frame[top:bottom, left:right]
            # Save the image of the face
            cv2.imwrite(f"ignored_face{ignoredFacesCounter}.jpg", face_roi)
            print("Photo captured!")
            # Add the face to the ignored list
            ignoredFaces.append(face_recognition.load_image_file(f"ignored_face{ignoredFacesCounter}.jpg"))
            ignoredFacesEncodings = [face_recognition.face_encodings(image)[0] for image in ignoredFaces]
            print("Ignored")
            message = 3
            if message != lastMessage: # Only send if the message is changed from the last state
                    ser.write(b"3")
                    lastMessage = 3
            
            
    # Function to display the 3 options for unknown persons
    def on_button_click(action):
        if action == "Open" :
            action_open()
        elif action == "Add":
            action_add()
        elif action == "Ignore" :
             action_ignore()
        dialog.destroy()

    # Custom dialog for decision
    dialog = tk.Toplevel(root)
    dialog.title("Unauthorized Person Detected")

    tk.Label(dialog, text="Unauthorized person detected! What would you like to do?").pack(padx=20, pady=10)
    tk.Button(dialog, text="Open", command=lambda: on_button_click("Open")).pack(side=tk.LEFT, padx=10)
    tk.Button(dialog, text="Add", command=lambda: on_button_click("Add")).pack(side=tk.LEFT, padx=10)
    tk.Button(dialog, text="Ignore", command=lambda: on_button_click("Ignore")).pack(side=tk.LEFT, padx=10)

    dialog.transient(root)
    dialog.grab_set()
    root.wait_window(dialog)

# Create the main window
root = tk.Tk()
root.title("Face Recognition System")

# Create a label to hold the camera feed
lmain = tk.Label(root)
lmain.pack()

# Main loop
while True:
    # Reading frames
    ret, frame = cap.read()

    # Ensure frame is valid
    if not ret:
        continue

    # Resizing the frames to process faster
    processingFrame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)
    processingFrame = cv2.cvtColor(processingFrame, cv2.COLOR_BGR2RGB)

    knownFlag = False
    unknownFlag = False
    unauthorised = False

    # Processing once every 20 frames for smoother feed
    if counter % 20 == 0:
        face_locations = face_recognition.face_locations(processingFrame)
        face_encodings = face_recognition.face_encodings(processingFrame, face_locations)

    for (top, right, bottom, left), face_encoding in zip(face_locations, face_encodings):
        top *= 4
        right *= 4
        bottom *= 4
        left *= 4

        matches = face_recognition.compare_faces(knownFacesEncodings, face_encoding)
        if ignoredFacesCounter > 0:
            ignoredMatches = face_recognition.compare_faces(ignoredFacesEncodings, face_encoding)

        if True in matches:
            first_match_index = matches.index(True)
            cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 2)
            cv2.putText(frame, knownNames[first_match_index], (left + 40, top - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)
            knownFlag = True
            unauthorised = False
        else:
            cv2.rectangle(frame, (left, top), (right, bottom), (0, 0, 255), 2)
            cv2.putText(frame, "Unknown", (left + 40, top - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 0, 255), 2)
            unauthorised = True
            unknownFlag = True
            if ignoredFacesCounter > 0 and True in ignoredMatches:
                unauthorised = False

    if unauthorised and counter % 20 == 0:
        handle_unauthorised(frame, top, right, bottom, left)

    # Display the frame in the GUI
    cv2image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    img = Image.fromarray(cv2image)
    imgtk = ImageTk.PhotoImage(image=img)
    lmain.imgtk = imgtk
    lmain.configure(image=imgtk)

    # Handle serial communication based on the face found
    if knownFlag and not unknownFlag: # Only send if the message is changed from the last state
        message = 1
        if message != lastMessage:
            ser.write(b"1")
            lastMessage = 1
    elif not knownFlag and unknownFlag: # Only send if the message is changed from the last state
        message = 4
        if message != lastMessage:
            ser.write(b"4")
            lastMessage = 4

    # Increment the frame counter
    counter += 1

    # Check for GUI window close event
    root.update_idletasks()
    root.update()

    # Check if 'e' key is pressed to exit
    # Close the camera feed if 'e' is pressed
    if cv2.waitKey(1) & 0xFF == ord('e'):
        ser.write(b'0')
        break

# Close everything
ser.close()
cap.release()
cv2.destroyAllWindows()
root.destroy()
