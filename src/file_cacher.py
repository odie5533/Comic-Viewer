# Copyright (C) David Bern
# See COPYRIGHT.md for details


import thread
import rarfile
import zipfile
import os
import pygame
from cStringIO import StringIO

IMG_EXTENSIONS = ['jpg', 'jpeg', 'gif', 'png', 'bmp']

"""
Caches files from folders and archives so they are ready for blitting
"""
class FileCacher:
    def __init__(self, filename=None):
        self.archive = None
        self.max_cache = 40
        self.idx = 0
        # lock ensures only one thread attempts to preload images
        self.lock = thread.allocate_lock()

        # order contains a list of filenames in the order which they
        # should be read
        self.order = None
        # images contains pygame images as a dict with the key being the filename
        self.images = {}

    def __len__(self):
        if self.order is None:
            return 0
        return len(self.order)
    
    def at_end(self):
        return self.idx + 1 == len(self)
    def at_home(self):
        return self.idx == 0

    """
    Loads an index into memory and returns the loaded pygame image
    """
    def _load_index(self, idx=None):
        if not self.order:
            print "No images are ordered."
            return
        if idx is None:
            idx = self.idx
        if idx >= len(self):
            idx = len(self)-1
        if idx < 0:
            idx = 0
        name = self.order[idx]
        if name not in self.images:
            sio = StringIO()
            data = self.archive.read(name)
            sio.write(data)
            del(data)
            sio.seek(0)
            try:
                if isinstance(name, unicode):
                    hint = name.encode('ascii','ignore')
                    self.images[name] = pygame.image.load(sio, hint)
                else:
                    hint = name.decode('utf-8').encode('ascii','ignore')
                    self.images[name] = pygame.image.load(sio, hint)
            except pygame.error:
                print "Unable to load image:", name
                print pygame.get_error()
                self.order.pop(idx)
                return None
        return self.images[name]
    
    def load_image_relative(self, skip=0):
        return self._load_index(self.idx + skip)

    """
    Makes the given index the current image and returns the pygame image
    """
    def goto_index(self, idx=None):
        if idx is not None:
            self.idx = idx
        if self.idx >= len(self):
            self.idx = len(self)-1
        if self.idx < 0:
            self.idx = 0
        self.garbage_collect()
        return self._load_index(self.idx)

    """
    Loads previous and next images into memory
    Call from a new thread
    """
    def preload(self):
        suc = self.lock.acquire(0)
        if not suc:
            return
        # If the comic is smaller than the max_cache, preload everything
        if len(self) <= self.max_cache:
            try:
                for i in range(0, len(self)):
                    self._load_index(i)
            except Exception as e:
                print e
                print "Error performing preload"
        # Otherwise, load half before and half after
        else:
            cache = self.max_cache / 2
            try:
                for i in range(self.idx, self.idx + cache + 1):
                    self._load_index(i)
                for i in range(self.idx - cache - 1, self.idx):
                    self._load_index(i)
            except:
                print "Error performing preload"
            self.garbage_collect()
        self.lock.release()
    
    def preload_in_thread(self):
        thread.start_new_thread(self.preload, ())

    """
    Cleans out old images beyond the cache_ahead and cache_behind limit
    """
    def garbage_collect(self):
        if len(self) <= self.max_cache:
            return
        cache = self.max_cache / 2
        for i in range(0, self.idx - cache - 1):
            name = self.order[i]
            if name in self.images:
                del self.images[name]
        for i in range(self.idx + cache + 1, len(self)):
            name = self.order[i]
            if name in self.images:
                del self.images[name]

    def progress_image(self, forward=True, skip=1):
        self.idx += skip if forward else (-skip)
        return self.goto_index(self.idx)

    def next_image(self, skip=1):
        return self.progress_image(True, skip)

    def prev_image(self, skip=1):
        return self.progress_image(False, skip)

    def goto_end(self):
        self.idx = len(self)-1
        return self.goto_index(self.idx)

    def goto_home(self):
        self.idx = 0
        return self.goto_index(self.idx)

    """
    The _load_index method does not support loading folder
    """
    def load_folder(self, folder):
        self.order = os.listdir(folder)
        self.order = [os.path.join(folder,e) for e in self.order]

    """
    Attempts to load either rar or zip archives.
    Then reads the archive for images.
    """
    def load_archive(self, filename):
        archive = None
        try:
            archive = rarfile.RarFile(filename)
        except rarfile.NotRarFile:
            pass
        if archive == None:
            try:
                archive = zipfile.ZipFile(filename)
            except zipfile.BadZipfile:
                pass
        if archive == None:
            print "Unable to open archive"
            return

        self.filename = filename
        self.archive = archive
        self.order = []
        for inf in self.archive.infolist():
            # ignores MAC crap
            if inf.filename.startswith("__MACOSX"):
                continue
            extension = os.path.splitext(inf.filename)[1][1:].lower()
            if extension in IMG_EXTENSIONS:
                self.order.append(inf.filename)
        self.order.sort(key=lambda s: s.lower())
        del self.images
        self.images = {}
