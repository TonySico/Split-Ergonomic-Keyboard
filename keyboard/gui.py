# Notes:
# 
# News / Updates:
# -Tool now supports <Shift> ... </Shift> functionality and <Home> in macros. Need to copy-paste-edit to add other modifiers and special keys.
#
# Bugs:
# -If you wanna switch which side is master, you need a method to flash slave code onto other
# -COM port isn't always COM4 on windows, so need to scan or ask user to check device manager
#
# Investigate / Test:
# -Can I just send '<' as a macro?
# -What happens if I send <LShift>,</LShift>LShift>? Will it register Shift+, as an open bracket?
# -Test if it's possible to flash left as master and right as slave (and document the procedure, maybe add popup to inform user)
#
# TODO:
# -I would like to add an info box with a link to either a large text document or perhaps just my email
# -Eventually I will add layer support, though perhaps I should leave it to Tony
# -prompt user to save flashed config when they hit flash (y/n then either prompt save windows or flash directly)
# -Add macro viewer

import tkinter 
import subprocess
import os
import zipfile
from tkinter import *
from tkinter import font as tkFont, Button, simpledialog, messagebox, filedialog
from PIL import Image, ImageTk
from functools import partial


# Used to create specials dialog box
class ListSelectionDialog(simpledialog.Dialog):
    def __init__(self, parent, title, options):
        self.options = options
        self.selected_value = None
        super().__init__(parent, title)

    def body(self, master):
        tkinter.Label(master, text="Select Special").pack()
        
        self.listbox = tkinter.Listbox(master, selectmode=tkinter.SINGLE)
        for option in self.options:
            self.listbox.insert(tkinter.END, option)
        self.listbox.pack()
        
        self.geometry("275x225")

        return self.listbox  # Initial focus

    def apply(self):
        selected_indices = self.listbox.curselection()
        if selected_indices:
            self.selected_value = self.options[selected_indices[0]]

# Used to input macros
class MacroInput(simpledialog.Dialog):
    def __init__(self, parent, title, specials_list, old_content=None):
        self.specials_list = specials_list
        self.old_content = old_content
        super().__init__(parent, title)

    def body(self, master):
        tkinter.Label(master, text="Enter macro:").pack(pady=5)
        self.entry = tkinter.Entry(master, width=50)

        if self.old_content:
            self.entry.insert(0, self.old_content)

        self.entry.pack(pady=5)
        return self.entry  

    def buttonbox(self):
        """Override the default button box to add custom buttons."""
        box = tkinter.Frame(self)

        ok_button = tkinter.Button(box, text="OK", width=10, command=self.ok)
        ok_button.pack(side=tkinter.LEFT, padx=5, pady=5)

        specials_button = tkinter.Button(box, text="Specials", width=10, command=self.specials)
        specials_button.pack(side=tkinter.LEFT, padx=5, pady=5)

        cancel_button = tkinter.Button(box, text="Cancel", width=10, command=self.cancel)
        cancel_button.pack(side=tkinter.LEFT, padx=5, pady=5)

        box.pack()

    def specials(self):
        self.dialog = ListSelectionDialog(root, "Select Special (scroll for more)", self.specials_list)
        if int(m[self.dialog.selected_value], 16) >= 0xE0: # Is it shift, alt, windows or ctrl?
            self.entry.insert(len(self.entry.get()), "<"+self.dialog.selected_value+"></"+self.dialog.selected_value+">")
        else: # Nope, just a regular special like "home"
            self.entry.insert(len(self.entry.get()), "<"+self.dialog.selected_value+">")

    def apply(self):
        for x in ["LShift>", "RShift>", "LCtrl>", "RCtrl>", "LAlt>", "RAlt>", "LWin>", "RWin>"]:
            if self.entry.get().count("<"+x) != self.entry.get().count("</"+x):
                messagebox.showerror("Invalid Macro", "Number of open and close special modifier tags (Shift, Ctrl, Alt, Win) must be equal!\nNumber of <"+str(x)+" not equal to number of </"+str(x))
                self.result = "\0" # error state, re-open window
                return
        if "\\0" in self.entry.get():
            messagebox.showerror("Invalid Macro", "Macros may not contain \"\\0\".")
            self.result = "\0" # error state, re-open window
        elif len(self.entry.get()) == 0:
            messagebox.showerror("Invalid Macro", "Please enter more than one char.")
            self.result = "\0" # error state, re-open window
        else: 
            self.result = self.entry.get() 

# Used to name macros
class MacroName(simpledialog.Dialog):
    def __init__(self, parent, title, macro_list, old_name=None):
        self.macros = macro_list
        self.old_name = old_name
        super().__init__(parent, title)

    def body(self, master):
        tkinter.Label(master, text="Enter macro name (8 chars max.): ").pack(pady=5)
        self.entry = tkinter.Entry(master)

        if self.old_name:
            self.entry.insert(0, self.old_name)

        self.entry.pack(pady=5)
        return self.entry  

    def apply(self):
        if len(self.entry.get()) == 0 or len(self.entry.get()) > 8:
            messagebox.showerror("Invalid Macro Name", "Please enter 2-8 chars.")
            self.result = "\0"
        elif self.entry.get() in m:
            messagebox.showerror("Invalid Macro Name", "Don't try to break me :`(\n Macros cannot be named the same as a valid key.")
            self.result = "\0"
        elif ":" in self.entry.get() or self.entry.get() == "???":
            messagebox.showerror("Invalid Macro Name", "Macro names may not contain \":\" and may not be \"???\".")
            self.result = "\0"
        elif self.entry.get() in self.macros:
            if messagebox.askquestion(title="Macro Already Exists", message="A macro named "+str(self.entry.get())+" already exists.\n\nOverwrite?") == "yes":
                self.result = self.entry.get() 
            else:
                self.result = "\0"
        else: 
            self.result = self.entry.get() 

# This is used to display keys which have double mappings (e.g. '1' maps to '1' and '!' when shifted)
special_doubles = {
    "-": "_",
    "=": "+",
    "[": "{",
    "]": "}",
    "\\": "|",
    "#": "~",
    ";": ":",
    "'": "\"",
    "`": "~",
    ",": "<",
    ".":  ">",
    "/": "?",
    "1":  "!",
    "2":  "@",
    "3":  "#",
    "4":  "$",
    "5":  "%",
    "6": "^",
    "7":  "&",
    "8": "*",
    "9": "(",
    "0": ")"}

# This is used to map from keys to USB codes
m = {
    "a": "0x04", "A": "0x04",
    "b": "0x05", "B": "0x05",
    "c": "0x06", "C": "0x06",
    "d": "0x07", "D": "0x07",
    "e": "0x08", "E": "0x08",
    "f": "0x09", "F": "0x09",
    "g": "0x0A", "G": "0x0A",
    "h": "0x0B", "H": "0x0B",
    "i": "0x0C", "I": "0x0C",
    "j": "0x0D", "J": "0x0D",
    "k": "0x0E", "K": "0x0E",
    "l": "0x0F", "L": "0x0F",
    "m": "0x10", "M": "0x10",
    "n": "0x11", "N": "0x11",
    "o": "0x12", "O": "0x12",
    "p": "0x13", "P": "0x13",
    "q": "0x14", "Q": "0x14",
    "r": "0x15", "R": "0x15",
    "s": "0x16", "S": "0x16",
    "t": "0x17", "T": "0x17",
    "u": "0x18", "U": "0x18",
    "v": "0x19", "V": "0x19",
    "w": "0x1A", "W": "0x1A",
    "x": "0x1B", "X": "0x1B",
    "y": "0x1C", "Y": "0x1C",
    "z": "0x1D", "Z": "0x1D",
    "1": "0x1E", "!": "0x1E",
    "2": "0x1F", "@": "0x1F",
    "3": "0x20", "#": "0x20",
    "4": "0x21", "$": "0x21",
    "5": "0x22", "%": "0x22",
    "6": "0x23", "^": "0x23", 
    "7": "0x24", "&": "0x24",
    "8": "0x25", "*": "0x25",
    "9": "0x26", "(": "0x26",
    "0": "0x27", ")": "0x27",
    "\n": "0x28", "\r": "0x28",
    "ESC": "0x29",
    "\b": "0x2A",  # Backspace
    "\t": "0x2B",  # Tab
    " ": "0x2C",  # Spacebar
    "-": "0x2D", "_": "0x2D",
    "=": "0x2E", "+": "0x2E",
    "[": "0x2F", "{": "0x2F",
    "]": "0x30", "}": "0x30",
    "\\": "0x31", "|": "0x31",
    "#": "0x32", "~": "0x32",  # Non-US # and ~
    ";": "0x33", ":": "0x33",
    "'": "0x34", '"': "0x34",
    "`": "0x35", "~": "0x35",  # Grave accent and tilde
    ",": "0x36", "<": "0x36",
    ".": "0x37", ">": "0x37",
    "/": "0x38", "?": "0x38",
    "Caps Lock": "0x39",
    "Enter": "0x28",  # Return (ENTER)
    "Escape": "0x29",
    "Backspace": "0x2A",
    "Tab": "0x2B",
    "Spacebar": "0x2C",
    "PrintScr": "0x46",  # PrintScreen
    "ScrollLck": "0x47",  # Scroll Lock
    "Pause": "0x48",  # Pause
    "Insert": "0x49",
    "Home": "0x4A",
    "PageUp": "0x4B",
    "Delete": "0x4C",
    "End": "0x4D",
    "PageDown": "0x4E",
    "RightArrow": "0x4F",
    "LeftArrow": "0x50",
    "DownArrow": "0x51",
    "UpArrow": "0x52",
    "NumLck": "0x53",  # Num Lock
    "KeypadEnter": "0x58",  # Keypad Enter
    "LCtrl": "0xE0",  # Left Control
    "LShift": "0xE1",  # Left Shift
    "LAlt": "0xE2",  # Left Alt
    "LWin": "0xE3",  # Left GUI (Windows)
    "RCtrl": "0xE4",  # Right Control
    "RShift": "0xE5",  # Right Shift
    "RAlt": "0xE6",  # Right Alt
    "RWin": "0xE7",  # Right GUI (Windows)
    "F1": "0x3A", 
    "F2": "0x3B",
    "F3": "0x3C",
    "F4": "0x3D",
    "F5": "0x3E",
    "F6": "0x3F",
    "F7": "0x40",
    "F8": "0x41",
    "F9": "0x42",
    "F10": "0x43",
    "F11": "0x44",
    "F12": "0x45",
    "F13": "0x68",
    "F14": "0x69",
    "F15": "0x6A",
    "F16": "0x6B",
    "F17": "0x6C",
    "F18": "0x6D",
    "F19": "0x6E",
    "F20": "0x6F",
    "F21": "0x70",
    "F22": "0x71",
    "F23": "0x72",
    "F24": "0x73"
}

# This is the main window
class MainWindow(Frame):
    def __init__(self, master, *pargs):
        Frame.__init__(self, master, *pargs)

        # Create Canvas
        self.canvas = Canvas(self, highlightthickness=0)
        self.canvas.pack(fill=BOTH, expand=YES)

        # Load Images:
        # region
        # Load the background image
        self.bg_image = Image.open("./resources/both_halves2.png")
        self.bg_copy = self.bg_image.copy()
        self.bg_tk_image = ImageTk.PhotoImage(self.bg_image) # Convert images for Tkinter

        # Load key button image with transparency (RGBA mode) and clicked version
        self.btn_image = Image.open("./resources/button2.png").convert("RGBA")  
        self.testButtonImg = ImageTk.PhotoImage(self.btn_image)
        # self.btn_image = Image.open("./resources/button2.png").convert("RGBA")  
        # self.testButtonImg = ImageTk.PhotoImage(self.btn_image)

        # Load flash button image with transparency (RGBA mode) and clicked version
        self.flash_image = Image.open("./resources/flash.png").convert("RGBA")  
        self.flashButtonImg = ImageTk.PhotoImage(self.flash_image)
        self.c_flash_image = Image.open("./resources/flash_clicked.png").convert("RGBA")  
        self.c_flashButtonImg = ImageTk.PhotoImage(self.c_flash_image)

        # Load defaults button image with transparency (RGBA mode) and clicked version
        self.default_image = Image.open("./resources/default.png").convert("RGBA")  
        self.defaultButtonImg = ImageTk.PhotoImage(self.default_image)
        self.c_default_image = Image.open("./resources/default_clicked.png").convert("RGBA")  
        self.c_defaultButtonImg = ImageTk.PhotoImage(self.c_default_image)

        # Load save button image with transparency (RGBA mode) and clicked version
        self.save_image = Image.open("./resources/save.png").convert("RGBA")  
        self.saveButtonImg = ImageTk.PhotoImage(self.save_image)
        self.c_save_image = Image.open("./resources/save_clicked.png").convert("RGBA")  
        self.c_saveButtonImg = ImageTk.PhotoImage(self.c_save_image)

        # Load "load" button image with transparency (RGBA mode) and clicked version
        self.load_image = Image.open("./resources/load.png").convert("RGBA")  
        self.loadButtonImg = ImageTk.PhotoImage(self.load_image)
        self.c_load_image = Image.open("./resources/load_clicked.png").convert("RGBA")  
        self.c_loadButtonImg = ImageTk.PhotoImage(self.c_load_image)
        # endregion

        # Add the background image to the canvas
        self.bg = self.canvas.create_image(0, 0, anchor=NW, image=self.bg_tk_image)

        # Keep track of which button I'm editing if any
        self.editing = -1

        # Keep track of which board is master
        self.masterBoard = "Left"

        # List of special keys
        # NOTE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # NOTE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # NOTE THESE ARE NOT JUST CHOSEN RANDOMLY, THE WORDS MUST MATCH EXACTLY WITH LOGIC HARDCODED IN THE FIRMWARE! DO NOT CHANGE THIS UNLESS YOU ALSO CHANGE FIRMWARE LOGIC!
        # NOTE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # NOTE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        self.specials = ["Custom (Macro)", "Caps Lock", "Enter", "Escape", "Backspace", "Tab", "Spacebar", "PrintScr", "ScrollLck", "Pause", "Insert", "Home", "PageUp", "Delete", "End", "PageDown", "RightArrow", "LeftArrow", "DownArrow", "UpArrow", "NumLck", "KeypadEnter", "LCtrl", "LShift", "LAlt", "LWin", "RCtrl", "RShift", "RAlt", "RWin", "F1 ", "F2 ", "F3 ", "F4 ", "F5 ", "F6 ", "F7 ", "F8 ", "F9 ", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24"]

        # print(len(self.specials))

        # List of macros (name and content)
        self.macros = {}

        # Create keys and labels for them
        self.KeycapFont = "DejaVu Sans"
        self.KeycapColor = "#5b5b5b"

        self.buttonImageList = []
        self.buttonLabelList = []
        for i in range(0, 60): 
            self.buttonImageList.append(self.canvas.create_image(100, 100, anchor="center", image=self.testButtonImg))
            self.canvas.tag_bind(self.buttonImageList[i], "<Button-1>", partial(self.OnButtonClick, i))

            self.buttonLabelList.append(self.canvas.create_text(self.canvas.coords(self.buttonImageList[i])[0], self.canvas.coords(self.buttonImageList[i])[1]-0.2*(self.canvas.bbox(self.buttonImageList[i])[3]-self.canvas.bbox(self.buttonImageList[i])[1]), anchor="center", text="\n???", font=(self.KeycapFont, 13, "bold"), fill=self.KeycapColor))
            self.canvas.tag_bind(self.buttonLabelList[i], "<Button-1>", partial(self.OnButtonClick, i))

        # Create other buttons 
        self.default=self.canvas.create_image(100, 100, anchor="center", image=self.defaultButtonImg)
        self.canvas.tag_bind(self.default, "<ButtonPress-1>", partial(self.OnDefaultClick))
        self.canvas.tag_bind(self.default, "<ButtonRelease-1>", lambda event: self.canvas.itemconfig(self.default, image=self.defaultButtonImg))

        self.flash=self.canvas.create_image(100, 100, anchor="center", image=self.flashButtonImg)
        self.canvas.tag_bind(self.flash, "<ButtonPress-1>", partial(self.onFlashClick))
        self.canvas.tag_bind(self.flash, "<ButtonRelease-1>", lambda event: self.canvas.itemconfig(self.flash, image=self.flashButtonImg))

        self.save=self.canvas.create_image(50, 50, anchor="center", image=self.saveButtonImg)
        self.canvas.tag_bind(self.save, "<ButtonPress-1>", partial(self.OnSaveClick))
        self.canvas.tag_bind(self.save, "<ButtonRelease-1>", lambda event: self.canvas.itemconfig(self.save, image=self.saveButtonImg))

        self.load=self.canvas.create_image(50, 50, anchor="center", image=self.loadButtonImg)
        self.canvas.tag_bind(self.load, "<ButtonPress-1>", partial(self.OnLoadClick))
        self.canvas.tag_bind(self.load, "<ButtonRelease-1>", lambda event: self.canvas.itemconfig(self.load, image=self.loadButtonImg))

        self.masterBoard = tkinter.StringVar()
        self.masterBoard.set("right")
        self.master_left = tkinter.Radiobutton(root, text="Left  ", value="left", variable=self.masterBoard, background="black", indicatoron=True, fg=self.KeycapColor, font=(self.KeycapFont, 20), anchor="w")
        self.master_right = tkinter.Radiobutton(root, text="Right", value="right", variable=self.masterBoard, background="black", indicatoron=True, fg=self.KeycapColor, font=(self.KeycapFont, 20), anchor="w")
        self.master_left_window = self.canvas.create_window(100, 50, window=self.master_left)
        self.master_right_window = self.canvas.create_window(100, 100, window=self.master_right)
        self.masterLabel = self.canvas.create_text(self.master_left.winfo_x(), self.master_left.winfo_y()+100, anchor="w", text="Master Selection", font=(self.KeycapFont, 20, "bold"), fill=self.KeycapColor)

        # Bind window resize and other events
        self.canvas.bind("<Configure>", self._resize)
        self.canvas.tag_bind(self.bg, "<Button-1>", self.on_background_click)
        self.canvas.bind("<Return>", self.on_background_click) 
        self.canvas.bind("<Escape>", self.on_background_click)

    # Used to resize elements when window is resized
    def _resize(self, event):
        """ Resize background and button images when the window resizes. """
        new_width = event.width
        new_height = event.height

        # Resize background
        self.bg_image = self.bg_copy.resize((new_width, new_height), Image.Resampling.LANCZOS)
        self.bg_tk_image = ImageTk.PhotoImage(self.bg_image)
        self.canvas.itemconfig(self.bg, image=self.bg_tk_image)
        self.canvas.config(width=new_width, height=new_height)

        # Resize button images
        self.testButtonImg = ImageTk.PhotoImage(self.btn_image.resize((int(new_width * 0.055), int(new_height * 0.11)), Image.Resampling.LANCZOS)) # Correct ratio is 1:2
        for i in range(0, 60):
            self.canvas.itemconfig(self.buttonImageList[i], image=self.testButtonImg)

        self.c_flashButtonImg = ImageTk.PhotoImage(self.c_flash_image.resize((int(new_width * 0.17), int(new_height * 0.1)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.flash, image=self.c_flashButtonImg)
        self.flashButtonImg = ImageTk.PhotoImage(self.flash_image.resize((int(new_width * 0.17), int(new_height * 0.1)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.flash, image=self.flashButtonImg)
        

        self.c_defaultButtonImg = ImageTk.PhotoImage(self.c_default_image.resize((int(new_width * 0.17), int(new_height * 0.1)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.default, image=self.c_defaultButtonImg)
        self.defaultButtonImg = ImageTk.PhotoImage(self.default_image.resize((int(new_width * 0.17), int(new_height * 0.1)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.default, image=self.defaultButtonImg)
        

        self.c_saveButtonImg = ImageTk.PhotoImage(self.c_save_image.resize((int(new_width * 0.075), int(new_height * 0.04)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.save, image=self.c_saveButtonImg)
        self.saveButtonImg = ImageTk.PhotoImage(self.save_image.resize((int(new_width * 0.075), int(new_height * 0.04)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.save, image=self.saveButtonImg)
        

        self.c_loadButtonImg = ImageTk.PhotoImage(self.c_load_image.resize((int(new_width * 0.075), int(new_height * 0.04)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.load, image=self.c_loadButtonImg)
        self.loadButtonImg = ImageTk.PhotoImage(self.load_image.resize((int(new_width * 0.075), int(new_height * 0.04)), Image.Resampling.LANCZOS))
        self.canvas.itemconfig(self.load, image=self.loadButtonImg)

        # Update element coordinates positions:

        # Keycap buttons
        # region 

        self.canvas.coords(self.buttonImageList[0], new_width * 0.052, new_height * 0.177)
        self.canvas.coords(self.buttonImageList[1], new_width * 0.052, new_height * 0.291) 
        self.canvas.coords(self.buttonImageList[2], new_width * 0.052, new_height * 0.405) 
        self.canvas.coords(self.buttonImageList[3], new_width * 0.052, new_height * 0.517) 
        
        self.canvas.coords(self.buttonImageList[4], new_width * 0.109, new_height * 0.148) 
        self.canvas.coords(self.buttonImageList[5], new_width * 0.109, new_height * 0.262) 
        self.canvas.coords(self.buttonImageList[6], new_width * 0.109, new_height * 0.374)
        self.canvas.coords(self.buttonImageList[7], new_width * 0.109, new_height * 0.488) 
        
        self.canvas.coords(self.buttonImageList[8], new_width * 0.166, new_height * 0.118) 
        self.canvas.coords(self.buttonImageList[9], new_width * 0.166, new_height * 0.232) 
        self.canvas.coords(self.buttonImageList[10], new_width * 0.166, new_height * 0.345) 
        self.canvas.coords(self.buttonImageList[11], new_width * 0.166, new_height * 0.459) 
        
        self.canvas.coords(self.buttonImageList[12], new_width * 0.224, new_height * 0.109) 
        self.canvas.coords(self.buttonImageList[13], new_width * 0.224, new_height * 0.223) 
        self.canvas.coords(self.buttonImageList[14], new_width * 0.224, new_height * 0.336) 
        self.canvas.coords(self.buttonImageList[15], new_width * 0.224, new_height * 0.450) 
        self.canvas.coords(self.buttonImageList[16], new_width * 0.224, new_height * 0.563) 
        
        self.canvas.coords(self.buttonImageList[17], new_width * 0.281, new_height * 0.126) 
        self.canvas.coords(self.buttonImageList[18], new_width * 0.281, new_height * 0.240) 
        self.canvas.coords(self.buttonImageList[19], new_width * 0.281, new_height * 0.354) 
        self.canvas.coords(self.buttonImageList[20], new_width * 0.281, new_height * 0.467) 
        self.canvas.coords(self.buttonImageList[21], new_width * 0.281, new_height * 0.58) 
        
        self.canvas.coords(self.buttonImageList[22], new_width * 0.338, new_height * 0.141) 
        self.canvas.coords(self.buttonImageList[23], new_width * 0.338, new_height * 0.254) 
        self.canvas.coords(self.buttonImageList[24], new_width * 0.338, new_height * 0.367) 
        self.canvas.coords(self.buttonImageList[25], new_width * 0.338, new_height * 0.48) 
        self.canvas.coords(self.buttonImageList[26], new_width * 0.338, new_height * 0.593)
        
        self.canvas.coords(self.buttonImageList[27], new_width * 0.396, new_height * 0.571) 
        self.canvas.coords(self.buttonImageList[28], new_width * 0.396, new_height * 0.684)

        self.canvas.coords(self.buttonImageList[29], new_width * 0.453, new_height * 0.62) 


        
        self.canvas.coords(self.buttonImageList[30], new_width * (1.0035 -  0.052), new_height * 0.174)
        self.canvas.coords(self.buttonImageList[31], new_width * (1.0035 -  0.052), new_height * 0.288)
        self.canvas.coords(self.buttonImageList[32], new_width * (1.0035 -  0.052), new_height * 0.402)
        self.canvas.coords(self.buttonImageList[33], new_width * (1.0035 -  0.052), new_height * 0.514)

        self.canvas.coords(self.buttonImageList[34], new_width * (1.0035 -  0.109), new_height * 0.145)
        self.canvas.coords(self.buttonImageList[35], new_width * (1.0035 -  0.109), new_height * 0.259)
        self.canvas.coords(self.buttonImageList[36], new_width * (1.0035 -  0.109), new_height * 0.371)
        self.canvas.coords(self.buttonImageList[37], new_width * (1.0035 -  0.109), new_height * 0.485)

        self.canvas.coords(self.buttonImageList[38], new_width * (1.0035 -  0.166), new_height * 0.118)
        self.canvas.coords(self.buttonImageList[39], new_width * (1.0035 -  0.166), new_height * 0.232)
        self.canvas.coords(self.buttonImageList[40], new_width * (1.0035 -  0.166), new_height * 0.345)
        self.canvas.coords(self.buttonImageList[41], new_width * (1.0035 -  0.166), new_height * 0.459)

        self.canvas.coords(self.buttonImageList[42], new_width * (1.0035 -  0.224), new_height * 0.109)
        self.canvas.coords(self.buttonImageList[43], new_width * (1.0035 -  0.224), new_height * 0.223)
        self.canvas.coords(self.buttonImageList[44], new_width * (1.0035 -  0.224), new_height * 0.336)
        self.canvas.coords(self.buttonImageList[45], new_width * (1.0035 -  0.224), new_height * 0.450)
        self.canvas.coords(self.buttonImageList[46], new_width * (1.0035 -  0.224), new_height * 0.563)
        
        self.canvas.coords(self.buttonImageList[47], new_width * (1.0035 -  0.281), new_height * 0.126)
        self.canvas.coords(self.buttonImageList[48], new_width * (1.0035 -  0.281), new_height * 0.240)
        self.canvas.coords(self.buttonImageList[49], new_width * (1.0035 -  0.281), new_height * 0.354)
        self.canvas.coords(self.buttonImageList[50], new_width * (1.0035 -  0.281), new_height * 0.467)
        self.canvas.coords(self.buttonImageList[51], new_width * (1.0035 -  0.281), new_height * 0.58)
        
        self.canvas.coords(self.buttonImageList[52], new_width * (1.0035 -  0.338), new_height * 0.141)
        self.canvas.coords(self.buttonImageList[53], new_width * (1.0035 -  0.338), new_height * 0.254)
        self.canvas.coords(self.buttonImageList[54], new_width * (1.0035 -  0.338), new_height * 0.367)
        self.canvas.coords(self.buttonImageList[55], new_width * (1.0035 -  0.338), new_height * 0.48)
        self.canvas.coords(self.buttonImageList[56], new_width * (1.0035 -  0.338), new_height * 0.593)

        self.canvas.coords(self.buttonImageList[57], new_width * (1.0035 -  0.396), new_height * 0.571)
        self.canvas.coords(self.buttonImageList[58], new_width * (1.0035 -  0.396), new_height * 0.684)

        self.canvas.coords(self.buttonImageList[59], new_width * (1.0035 -  0.453), new_height * 0.62)
        
        # endregion

        # Keycap text labels
        for i in range(len(self.buttonLabelList)): 
            self.canvas.coords(self.buttonLabelList[i], self.canvas.coords(self.buttonImageList[i])[0], self.canvas.coords(self.buttonImageList[i])[1]-0.2*(self.canvas.bbox(self.buttonImageList[i])[3]-self.canvas.bbox(self.buttonImageList[i])[1]))

        # Other buttons
        self.canvas.coords(self.default, new_width * 0.625, new_height * 0.9)
        self.canvas.coords(self.flash, new_width * 0.375, new_height * 0.9)
        self.canvas.coords(self.master_left_window, new_width *0.05, new_height * 0.85)
        self.canvas.coords(self.master_right_window, new_width *0.05, new_height * 0.925)
        self.canvas.coords(self.masterLabel, new_width * 0.01, new_height * 0.8)
        self.canvas.coords(self.save, new_width * 0.925, new_height * 0.85)
        self.canvas.coords(self.load, new_width * 0.925, new_height * 0.925)

        # Entry box and specials button (if currently editing) ERROR
        if self.editing != -1:
            self.canvas.coords(self.entry_window, self.canvas.coords(self.buttonImageList[self.editing])[0], self.canvas.coords(self.buttonImageList[self.editing])[1])
            self.canvas.coords(self.specialButton_window, self.canvas.coords(self.entry_window)[0]+0.4*self.entry.winfo_width(), self.canvas.coords(self.entry_window)[1])

    # Used to generate entry box for user to type desired key mapping when a button is clicked
    def OnButtonClick(self, num, event=None):
        """ Handle button click """
        print(f"Button {num} clicked!")

        if self.editing != -1: #what else are we editing
            self.on_background_click()

        self.editing = num
        
        # Create the Entry widget with current button text
        self.entry = Entry(self, font=(self.KeycapFont, 14), bd=3, bg="black", fg='gray', width=12)
        self.entry.insert(0, self.canvas.itemcget(self.buttonLabelList[num], "text"))
        self.entry_window = self.canvas.create_window(100, 100, window=self.entry)
        self.canvas.coords(self.entry_window, self.canvas.coords(self.buttonImageList[num])[0], self.canvas.coords(self.buttonImageList[num])[1])

        self.entry.update_idletasks()

        self.specialButton = Button(self, borderwidth=3, text="Special", command=self.onSpecialClick, bg="gray", fg='white', font=tkFont.Font(family=self.KeycapFont, size=11, weight=tkFont.BOLD))
        self.specialButton_window = self.canvas.create_window(100, 100, window=self.specialButton)
        self.canvas.coords(self.specialButton_window, self.canvas.coords(self.entry_window)[0]+0.4*self.entry.winfo_width(), self.canvas.coords(self.entry_window)[1])

        self.entry.bind('<Return>', self.on_background_click)
        self.entry.bind('<Escape>', self.on_background_click)

        # Focus on the Entry widget so the user can start typing
        self.entry.focus()

    # Used to generate a dialog box when the user hits "Specials" to allow them to map special keys (e.g. Scroll Lock or Caps Lock)
    def onSpecialClick(self):
        self.dialog = ListSelectionDialog(root, "Select Special (scroll for more)", self.specials)
        
        if self.dialog.selected_value == None:
            self.on_background_click()
            return
        elif self.dialog.selected_value == "Custom (Macro)":
            macro_content = None
            macro_name = None
            if self.canvas.itemcget(self.buttonLabelList[self.editing], "text").splitlines()[1] in self.macros: # was already a macro, want to overwrite or change
                messagebox.showinfo(title="Info", message="You are editing an existing macro. You may edit it or accept its contents and name as they are to leave un affected")
                macro_content = self.macros[self.canvas.itemcget(self.buttonLabelList[self.editing], "text").splitlines()[1]]
                macro_name = self.canvas.itemcget(self.buttonLabelList[self.editing], "text").splitlines()[1]

            returned = "\0"
            while(returned == "\0"):
                content_dialog = MacroInput(root, "Custom (Macro)", self.specials[1:], macro_content)
                returned = content_dialog.result
            if returned: # good input
                macro_content = returned
            else: # they cancelled
                self.on_background_click()
                return

            returned = "\0"
            while(returned == "\0"):
                naming_dialog = MacroName(root, "Custom (Macro)", self.macros, macro_name)
                returned = naming_dialog.result
            if returned: # they wrote something
                macro_name = returned
            else: # they cancelled
                self.on_background_click()
                return

            self.macros[macro_name] = macro_content
            self.entry.delete(0, tkinter.END)
            self.entry.insert(0, macro_name)
        else:
            self.entry.delete(0, tkinter.END)
            self.entry.insert(0, self.dialog.selected_value)
        self.on_background_click()

    # Used to lock in mapping when user clicks off entry box, hits enter, hits escape or selects a special in the special dialog box
    def on_background_click(self, event=None):
        """Save the text when clicking anywhere else on the canvas."""
        if self.editing != -1:
            # Get the edited text from the Entry widget
            self.button_text = self.entry.get().replace("\n", "")
            
            self.entry.delete(0, tkinter.END)
            self.entry.insert(0, "")

            # Is this a macro?
            if self.button_text in self.macros:
                self.canvas.itemconfig(self.buttonLabelList[self.editing], font=(self.KeycapFont, 9, "bold"), fill="#e6e300")
                self.button_text = "\n"+self.button_text
            else:
                # Set to default font size for regular key
                self.canvas.itemconfig(self.buttonLabelList[self.editing], font=(self.KeycapFont, 13, "bold"), fill=self.KeycapColor)

                # Special case for the idiot who will type a literal " " instead of using Specials>Spacebar or typing "Spacebar"
                if self.button_text == " ":
                    self.button_text = "Spacebar"

                # If text is user-entered and not a real key:
                if len(self.button_text) != 1 and self.button_text not in self.specials:
                    self.button_text = "???" 
                elif self.button_text in self.specials: # If text is a special (user-entered or selected with specials menu)
                    self.canvas.itemconfig(self.buttonLabelList[self.editing], font=(self.KeycapFont, 9, "bold"))
                else: # not special, so make it caps
                    self.button_text = self.button_text.upper()

                # Deal with special doubles
                if self.button_text in special_doubles:
                    self.button_text = special_doubles[self.button_text]+"\n"+self.button_text
                elif self.button_text in special_doubles.values():
                    self.button_text = self.button_text+"\n"+str(next((k for k, v in special_doubles.items() if v == self.button_text), None))
                else:
                    self.button_text = "\n"+self.button_text

            
            # Remove the Entry widget and return to the Button
            self.canvas.delete(self.entry_window)

            # Write new label contents
            self.canvas.itemconfig(self.buttonLabelList[self.editing], text=self.button_text)

            self.canvas.delete(self.specialButton_window)
            
            # Exit edit mode
            self.editing = -1

    # Used to generate master selection prompt, convert from mapping to USB encoded bytes and flash firmware
    def onFlashClick(self, event=None):
        self.canvas.itemconfig(self.flash, image=self.c_flashButtonImg)
        if self.editing != -1: #what else are we editing
            self.on_background_click()

        # Extract all key mappings into an array
        k = [self.canvas.itemcget(x, "text").splitlines()[1] for x in self.buttonLabelList]

        # Check that all keys have SOME assigned value, else kick user back
        if "\n???" in k or "???" in k:
            for x in self.buttonLabelList:
                if self.canvas.itemcget(x, "text") == "\n???":
                    self.canvas.itemconfig(x, fill="red")
            return
        
        # Count number of macros
        macro_count = 0
        for i in k:
            if i in self.macros:
                macro_count += 1
        if macro_count > 8:
            tkinter.messagebox.showerror(title="Error", message="A maximum of 8 macros are supported (you have configured "+str(macro_count)+")")
            return
        
        # Keep track / count of macros
        current_macro_mapping = 0xE8 # goes up to 0xEF (8 in total)

        # This is the USB encoded mappings
        mapping = []

        if self.masterBoard == "Right":
            mapping=[k[0], k[4], k[8], k[12], k[17], k[22],   k[1], k[5], k[9], k[13], k[18], k[23],    k[2], k[6], k[10], k[14], k[19], k[24],     k[3], k[7], k[11], k[15], k[20], k[25],    k[16], k[21], k[26], k[28], k[27], k[29],
                    k[30], k[34], k[38], k[42], k[47], k[52],   k[31], k[35], k[39], k[43], k[48], k[53],    k[32], k[36], k[40], k[44], k[49], k[54],     k[33], k[37], k[41], k[45], k[50], k[55],    k[46], k[51], k[56], k[58], k[57], k[59]]
        else:
            mapping=[k[30], k[34], k[38], k[42], k[47], k[52],   k[31], k[35], k[39], k[43], k[48], k[53],    k[32], k[36], k[40], k[44], k[49], k[54],     k[33], k[37], k[41], k[45], k[50], k[55],    k[46], k[51], k[56], k[58], k[57], k[59],
                     k[0], k[4], k[8], k[12], k[17], k[22],   k[1], k[5], k[9], k[13], k[18], k[23],    k[2], k[6], k[10], k[14], k[19], k[24],     k[3], k[7], k[11], k[15], k[20], k[25],    k[16], k[21], k[26], k[28], k[27], k[29]]
        
        # Write out mapping file
        if os.name == 'nt': # windows
            f = open("firmware\src\mapping.c", "w+")
        elif os.name == 'posix': # linux
            f = open("firmware/src/mapping.c", "w+")
        f.write("#include \"mapping.h\"\n\n")
        f.write("const uint8_t mapping[2*NUM_IN_PIN][NUM_OUT_PIN] =\n")
        f.write("{   {")
        for i in range(len(mapping)):
            if i==30:
                f.write("},\n    {")
            elif not (i % 6) and i:
                f.write("}, {")
            elif i:
                f.write(", ")
            if mapping[i] in self.macros:
                f.write(str(hex(current_macro_mapping)))
                current_macro_mapping += 1
            else:
                f.write(str(m[mapping[i]]))
        f.write("}};\n\n// Generated by software. First row is slave mappings, second is master's.")            
        f.close()

        # Write out macro file
        buffer = ""
        if os.name == 'nt': # windows
            f = open("firmware\src\macros.c", "w+")
        elif os.name == 'posix': # linux
            f = open("firmware/src/macros.c", "w+")
        f.write("#include \"macros.h\"\n\n")
        f.write("const char *macros["+str(macro_count)+"] = {")
        for i in mapping:
            if i in self.macros:
                buffer += "\""+str(self.macros[i])+"\", "
        buffer = buffer[:-2]
        f.write(buffer)
        f.write("};\n\n// Generated by software.")            
        f.close()

        # Begin flashing process
        if os.name == 'nt': # windows
            # Check if required avr-gcc files present
            if os.path.exists("firmware/avr8-gnu-toolchain.zip") and not os.path.exists("firmware//avr8-gnu-toolchain/"):
                with zipfile.ZipFile("firmware/avr8-gnu-toolchain.zip", 'r') as zip_ref:
                    zip_ref.extractall("firmware")
            elif not os.path.exists("firmware/avr8-gnu-toolchain.zip"):
                tkinter.messagebox.showerror(title="Error", message="Required avrgcc toolchain (for Windows) not present. Please download \"AVR 8-Bit Toolchain (Windows)\" from https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers and place .zip folder in guiTool/firmware")
                return
            # Run flashing script
            subprocess.run(["start", "cmd", "/K", "firmware\\flash.cmd"], shell=True)
            tkinter.messagebox.showinfo(title="Info", message="Steps: \n1) Connect the appropriate keyboard half via USB-C\n2) When avrdude is waiting for a device (\"avrdude: No device present\"), reset your master keyboard (you may also need to hit <Enter> in the command prompt)\n3) Once flashing is complete, reset the slave half then the master half to use newly mapped keyboard\n4) You may close the command prompt once flashing is complete")
        elif os.name == 'posix': # linux
            tkinter.messagebox.showinfo(title="Info", message="Note: avrdude and avr-gcc MUST be installed when flashing via Linux.\nSteps: \n1) Connect the appropriate keyboard half via USB-C\n2) When avrdude is waiting for a device (\"avrdude: No device present\"), reset your master keyboard\n3) Once flashing is complete, reset the slave half then the master half to use newly mapped keyboard\n4) You may close the command prompt once flashing is complete")
            os.chdir("./firmware/src/")
            subprocess.run(['make', 'makefile', 'master_flash'])
            os.chdir("../../")
        else: # Unknown
            tkinter.messagebox.showerror(title="Error", message="OS unrecognized, please flash manually.")

    # Used to reset key mappings to default values
    def OnDefaultClick(self, event=None):
        self.canvas.itemconfig(self.default, image=self.c_defaultButtonImg)
        if self.editing != -1: #what else are we editing
            self.on_background_click()

        # Set default values

        # region

        self.canvas.itemconfig(self.buttonLabelList[0], text="Escape")
        self.canvas.itemconfig(self.buttonLabelList[1], text="Tab")
        self.canvas.itemconfig(self.buttonLabelList[2], text="Caps Lock")
        self.canvas.itemconfig(self.buttonLabelList[3], text="LShift")

        self.canvas.itemconfig(self.buttonLabelList[4], text="1")
        self.canvas.itemconfig(self.buttonLabelList[5], text="Q")
        self.canvas.itemconfig(self.buttonLabelList[6], text="A")
        self.canvas.itemconfig(self.buttonLabelList[7], text="Z")

        self.canvas.itemconfig(self.buttonLabelList[8], text="2")
        self.canvas.itemconfig(self.buttonLabelList[9], text="W")
        self.canvas.itemconfig(self.buttonLabelList[10], text="S")
        self.canvas.itemconfig(self.buttonLabelList[11], text="X")

        self.canvas.itemconfig(self.buttonLabelList[12], text="3")
        self.canvas.itemconfig(self.buttonLabelList[13], text="E")
        self.canvas.itemconfig(self.buttonLabelList[14], text="D")
        self.canvas.itemconfig(self.buttonLabelList[15], text="C")
        self.canvas.itemconfig(self.buttonLabelList[16], text="Home")

        self.canvas.itemconfig(self.buttonLabelList[17], text="4")
        self.canvas.itemconfig(self.buttonLabelList[18], text="R")
        self.canvas.itemconfig(self.buttonLabelList[19], text="F")
        self.canvas.itemconfig(self.buttonLabelList[20], text="V")
        self.canvas.itemconfig(self.buttonLabelList[21], text="End")

        self.canvas.itemconfig(self.buttonLabelList[22], text="5")
        self.canvas.itemconfig(self.buttonLabelList[23], text="T")
        self.canvas.itemconfig(self.buttonLabelList[24], text="G")
        self.canvas.itemconfig(self.buttonLabelList[25], text="B")
        self.canvas.itemconfig(self.buttonLabelList[26], text="LWin")

        self.canvas.itemconfig(self.buttonLabelList[27], text="LCtrl")
        self.canvas.itemconfig(self.buttonLabelList[28], text="Spacebar")

        self.canvas.itemconfig(self.buttonLabelList[29], text="LAlt")


        self.canvas.itemconfig(self.buttonLabelList[30], text="=")
        self.canvas.itemconfig(self.buttonLabelList[31], text="[")
        self.canvas.itemconfig(self.buttonLabelList[32], text="]")
        self.canvas.itemconfig(self.buttonLabelList[33], text="'")

        self.canvas.itemconfig(self.buttonLabelList[34], text="0")
        self.canvas.itemconfig(self.buttonLabelList[35], text="P")
        self.canvas.itemconfig(self.buttonLabelList[36], text=";")
        self.canvas.itemconfig(self.buttonLabelList[37], text="/")

        self.canvas.itemconfig(self.buttonLabelList[38], text="9")
        self.canvas.itemconfig(self.buttonLabelList[39], text="O")
        self.canvas.itemconfig(self.buttonLabelList[40], text="L")
        self.canvas.itemconfig(self.buttonLabelList[41], text=".")

        self.canvas.itemconfig(self.buttonLabelList[42], text="8")
        self.canvas.itemconfig(self.buttonLabelList[43], text="I")
        self.canvas.itemconfig(self.buttonLabelList[44], text="K")
        self.canvas.itemconfig(self.buttonLabelList[45], text=",")
        self.canvas.itemconfig(self.buttonLabelList[46], text="Enter")

        self.canvas.itemconfig(self.buttonLabelList[47], text="7")
        self.canvas.itemconfig(self.buttonLabelList[48], text="U")
        self.canvas.itemconfig(self.buttonLabelList[49], text="J")
        self.canvas.itemconfig(self.buttonLabelList[50], text="M")
        self.canvas.itemconfig(self.buttonLabelList[51], text="Delete")

        self.canvas.itemconfig(self.buttonLabelList[52], text="6")
        self.canvas.itemconfig(self.buttonLabelList[53], text="Y")
        self.canvas.itemconfig(self.buttonLabelList[54], text="H")
        self.canvas.itemconfig(self.buttonLabelList[55], text="N")
        self.canvas.itemconfig(self.buttonLabelList[56], text="Backspace")

        self.canvas.itemconfig(self.buttonLabelList[57], text="RCtrl")
        self.canvas.itemconfig(self.buttonLabelList[58], text="Spacebar")

        self.canvas.itemconfig(self.buttonLabelList[59], text="RShift")
        
        # endregion

        # Adjust font sizes and spacing for specials
        for button in self.buttonLabelList:
            self.canvas.itemconfig(button, fill=self.KeycapColor)

            if self.canvas.itemcget(button, "text") in self.specials:
                self.canvas.itemconfig(button, text="\n"+self.canvas.itemcget(button, "text"), font=(self.KeycapFont, 9, "bold"))
            else:
                self.canvas.itemconfig(button, text=self.canvas.itemcget(button, "text").upper(), font=(self.KeycapFont, 13, "bold"))
                if self.canvas.itemcget(button, "text") in special_doubles: # Deal with special doubles
                    self.canvas.itemconfig(button, text=special_doubles[self.canvas.itemcget(button, "text")]+"\n"+self.canvas.itemcget(button, "text"))                      
                elif self.canvas.itemcget(button, "text") in special_doubles.values():
                    self.canvas.itemconfig(button, text=self.canvas.itemcget(button, "text")+"\n"+str(next((k for k, v in special_doubles.items() if v == self.canvas.itemcget(button, "text")), None)))
                else:
                    self.canvas.itemconfig(button, text="\n"+self.canvas.itemcget(button, "text"))
       
    # Used to save current mapping
    def OnSaveClick(self, event=None):
        if self.editing != -1: #what else are we editing
            self.on_background_click()

        self.canvas.itemconfig(self.save, image=self.c_saveButtonImg)

        save_path = filedialog.asksaveasfilename(title="Save File As", defaultextension=".map", filetypes=[(".map Files", "*.map"), ("All Files", "*.*")])
        if not save_path:
            tkinter.messagebox.showerror(title="Error", message="No location specified, mapping not saved!")
            return
        
        k = [self.canvas.itemcget(x, "text").splitlines()[1] for x in self.buttonLabelList]
        
        # Write one line for key mapping and then the rest are macro contents
        f = open(save_path, "w+")
        buffer = ""
        for i in k:
            buffer += str(i)+":"
        buffer = buffer[:-1]
        f.write(buffer)     

        buffer = "\n"
        for i in k:
            if i in self.macros:
                buffer += self.macros[i]+"\n"
        buffer = buffer[:-1]
        f.write(buffer) 
        f.close()

        tkinter.messagebox.showinfo(title="Save Successful", message="Mapping saved successfully to: "+str(save_path))

    # Used to load a previously saved mapping
    def OnLoadClick(self, event=None):
        if self.editing != -1: #what else are we editing
            self.on_background_click()

        self.canvas.itemconfig(self.load, image=self.c_loadButtonImg)

        file_path = filedialog.askopenfilename(title="Select a File", filetypes=[(".map Files", "*.map"), ("All Files", "*.*")])
        if not file_path:
            tkinter.messagebox.showerror(title="Error", message="No location specified, no mapping loaded!")
            return

        f = open(file_path, "r")
        buffer = f.readline().split(":")
        buffer[-1] = buffer[-1].strip()
        macro_contents = [x.strip() for x in f.readlines()]
        f.close()

        if len(buffer) != 60:
            tkinter.messagebox.showerror(title="Error", message="Invalid mapping read, operation aborted!")
            return

        for i in range(0, 60):
            self.canvas.itemconfig(self.buttonLabelList[i], fill=self.KeycapColor)

            if buffer[i] in self.specials: # Special
                self.canvas.itemconfig(self.buttonLabelList[i], text="\n"+buffer[i], font=(self.KeycapFont, 9, "bold"))
            elif buffer[i] not in m and buffer[i] != "???": # Macro
                self.canvas.itemconfig(self.buttonLabelList[i], text="\n"+buffer[i], font=(self.KeycapFont, 9, "bold"), fill="#e6e300")
                self.macros[buffer[i]] = macro_contents.pop(0)
            else: # Not macro, not special (regular key)
                self.canvas.itemconfig(self.buttonLabelList[i], text=buffer[i].upper(), font=(self.KeycapFont, 13, "bold"))
                if buffer[i] in special_doubles: # Deal with special doubles
                    self.canvas.itemconfig(self.buttonLabelList[i], text=special_doubles[buffer[i]]+"\n"+buffer[i])                      
                elif buffer[i] in special_doubles.values():
                    self.canvas.itemconfig(self.buttonLabelList[i], text=buffer[i]+"\n"+str(next((k for k, v in special_doubles.items() if v == buffer[i]), None)))
                else:
                    self.canvas.itemconfig(self.buttonLabelList[i], text="\n"+buffer[i])

# Create main window
root = Tk()
root.title("Key Mapping Tool")
# root.geometry("2031x1028")  # Starting size
root.geometry("1760x890")

e = MainWindow(root)
e.pack(fill=BOTH, expand=YES)

root.mainloop()
