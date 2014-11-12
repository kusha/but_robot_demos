#!/usr/bin/env python3

"""
Dialog manager library.

"""
__author__ = "Mark Birger"

import re
import sys, ast
import platform, os

class Dialog:
    """
    Dialog system class.
    Use this module to easily create voice
    interface to your functionality.
    """
    def __init__(self, scope, debug=False, silent=False):
        self.debug_flag = debug
        self.silent_flag = silent
        self.filenames = []
        self.functions = {}
        self.identation = "\t"
        self.automata = {}
        self.scope = {}
        self.pickup(scope)

    def load(self, filename):
        """
        Loads new .dlg files. Calls parser method.
        """
        self.filenames += filename
        self.parse(filename)

    def debug(self, text):
        """
        Method for printing debug messages.
        """
        if self.debug_flag:
            print("[DEBUG]: "+str(text))

    def info(self, text):
        """
        Method prints status messages of dialog system.
        May be usefull to control status in realese.
        """
        if not self.silent_flag:
            print("[INFOS]: "+str(text))

    def parse(self, filename):
        """
        Method opens file in dialog format. Parses it.
        Parsing may take a while, because this method
        also calls NLP parser.
        """
        self.debug("Pasing " + filename + " file")
        with open(filename) as dialog:
            content = dialog.read().splitlines()
        stack = [self.automata]
        current_depth = 0
        for line in content:
            line = self.remove_comments(line)
            if line is None:
                continue
            depth, line = self.count_identations(line)
            # TODO: parse setters # initial = self.parse_setters(line) 
            if current_depth == depth:
                stack[-1][line] = {}
                stack.append(stack[-1][line])
                current_depth += 1
            elif current_depth > depth:
                while not current_depth == depth:
                    stack.pop()
                    current_depth -= 1
                stack[-1][line] = {}
                stack.append(stack[-1][line])
                current_depth += 1
            else:
                self.info("Identation error in " + filename + "at line" + line)
        self.debug(filename + " parsed")
        self.debug(self.automata)

    # TODO: not finished
    def parse_setters(self, line):
        """
        Method called from parser, creates a dictionary
        of variable changes of two types: fixed, flexible.
        """
        result = {}
        setters1, line = self.parse_fixed_setters(line)
        result.update(setters1)
        setters2, line = self.parse_flexible_setters(line)
        result.update(setters2)
        return {"_set":result}, line

    # TODO: not finished
    @staticmethod
    def parse_fixed_setters(line):
        """
        Extends dictionary of setters to fixed values.
        Key is variable name. Value is evaluated literal.
        """
        regex = re.compile(r"`([^`]*):([^`]*[^\?])`")
        setters = {}
        for setter in regex.finditer(line):
            setters[setter.group(1)] = ast.literal_eval(setter.group(2))
            regex = re.compile(r"`([^`]*):([^`]*[^\?])`")
        re.sub(regex, "", line)
        return setters, line

    # TODO: not finished
    @staticmethod
    def parse_flexible_setters(line):
        """
        Method parses `___~___` expressions.
        Extends dictionary of setters. Keys are word indexes.
        Values are names of the variables.
        """
        regex = re.compile(r"`([^`]*)~([^`]*[^\?])`")
        setters = {}
        for idx, setter in enumerate(regex.finditer(line)):
            setters[idx] = setter.group(1)
            setters[setter.group(1)] = ast.literal_eval(setter.group(2))
        return {"_set": setters}

    # TODO: apply setters method

    @staticmethod
    def remove_comments(line):
        """
        Method removes comments and checks line to content.
        Returns None if line is empty.
        """
        line = re.sub(r"#.*", "", line)
        return line if line.strip() else None

    def count_identations(self, line):
        """
        Method counts identations at the start of line.
        """
        depth = 0
        while line.startswith(self.identation):
            depth += 1
            line = line[len(self.identation):]
        return depth, line

    def pickup(self, scope):
        """
        Method extract variable and functions from the
        global scope of caller file.
        """
        for name, obj in scope.items():
            if not name.startswith('__') and name != "Dialog":
                self.debug("Object " + name + " imported")
                self.scope[name] = obj

    def answer(self, phrase):
        """
        Answer result. Uses TTS engine.
        """
        self.info("Saying: " + phrase)
        if platform.system() == "Darwin":
            os.popen("say -v Alex \""+phrase+"\"")
        # TODO: other platfomrs
        # TODO: online TTS
        # TODO: audio_play at ROS

    def interprete(self):
        """
        Method interprets parsed aurtomata. Creates answers.
        """
        # self.set_variale('name', 'Mark')
        # print(self.call("hello"))
        while True:
            phrase = input()
            self.info("Got input: " + phrase)
            answers = self.resolve_input(phrase)
            if answers is not None: # TODO: update after resolve_input
                for answer in answers:
                    self.debug("Preprocessing " + answer)
                    answer = self.preprocess(answer)
                    self.answer(answer)
            break

    def resolve_input(self, phrase):
        """
        Resolves state for current input.
        """
        if phrase in self.automata:
            return self.automata[phrase]
        else:
            # TODO: Trigger if no state found
            exit()

    def preprocess(self, answer):
        """
        Evaluates inline variables and function calls inside phrase.
        """
        regex_inline = re.compile(r"`[^`]*`")
        result = ""
        prev = 0
        for inline in regex_inline.finditer(answer):
            value = self.evaluate(inline.group()[1:-1])
            self.debug(inline.group()[1:-1] + " is " + value)
            result += answer[:inline.start()-prev]
            result += value
            answer = answer[inline.end()-prev:]
            prev = inline.start() + len(inline.group())
        result += answer
        return result

    def evaluate(self, name):
        """
        Evaluates variables/fucntion calls.
        """
        if name in self.scope:
            if hasattr(self.scope[name], '__call__'):
                return str(self.call(name))
            else:
                return str(self.scope[name])
        else:
            self.info(name + " requested from the dialog not defined")
            return None

    def call(self, name):
        """
        Safety calls function from global scope. Locking dialog manager.
        """
        try:
            return self.scope[name]()
        except NameError:
            self.info("Can't evaluate " + name)
            sys.exit()

    def check_input(self, phrase, quality):
        """
        Controls thresholding of recognized phrase.
        Controls structure of recognized sentence.
        """
        # TODO: Say to quality function about quality 
        # TODO: Recognize input structure and check for meaning
        pass

    def execute(self, name):
        # TODO: make call in thread and add state/queue
        pass
