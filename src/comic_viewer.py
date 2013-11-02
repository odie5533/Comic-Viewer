# Copyright (C) David Bern
# See COPYRIGHT.md for details


"""
This program contains two classes: FileCacher and ComicViewer
The FileCacher class loads an archive and tracks the viewed image
It also caches images for quick load times and garbage collects old images

The ComicViewer class creates a pygame display and blits the comic image
onto the screen using the blit_image method. This also handles scaling oversized
images and scrolling around on an image
"""

import pygame
import pygame._view
import os
import sys
import re
from file_cacher import FileCacher

if os.name == 'nt':
    import win32gui
else:
    Tkinter=__import__("Tkinter")
    tkFileDialog=__import__("tkFileDialog")
    

TICKS_FOR_STATS = 20 # ticks to keep stats onscreen after changing image
STATUS_FONT = "Georgia"
STATUS_FONT_SIZE = 12

class ComicViewer:
    def GetWindowRect(self):
        if os.name == 'nt':
            hwnd = pygame.display.get_wm_info()['window']
            return pygame.Rect(win32gui.GetWindowRect(hwnd))
        return self.screen.get_rect()

    def GetClientRect(self):
        if os.name == 'nt':
            hwnd = pygame.display.get_wm_info()['window']
            return pygame.Rect(win32gui.GetClientRect(hwnd))
        return self.screen.get_rect()

    def __init__(self, archive_name):
        self.fullscreen = False
        self.twoup = False

        self.filecacher = FileCacher()

        self.clock = pygame.time.Clock()
        pygame.init()
        self.desktop_size = pygame.display.list_modes()[0]
        self.screen = pygame.display.set_mode(self.desktop_size,
                                              pygame.RESIZABLE, 32)
        self.ticks_till_blit = None
        self.load_archive(archive_name)

        if os.name == 'nt':
            # Move and resize the window to a nice default aspect ratio
            wrect = self.GetWindowRect()
            hwnd = pygame.display.get_wm_info()['window']

            # Here we are determining the 'offset' or difference between the
            # WindowRect and the ClientRect so that we can MoveWindow to a known
            # ClientRect size
            win32gui.MoveWindow(hwnd, wrect[0], wrect[1], 150, 150, True)
            wrect = self.GetWindowRect()
            crect = self.GetClientRect()
            self.offset = (wrect[2]-wrect[0]-crect[2],
                           wrect[3]-wrect[1]-crect[3])
            aspect_ratio = (float(self.image.get_size()[0]) /
                           self.image.get_size()[1])
            INITIAL_WIDTH = 600
            ceil = lambda x: int(x) + 1 if x - int(x) > 0 else int(x)
            scale_height = ceil(INITIAL_WIDTH / aspect_ratio)

            win32gui.MoveWindow(hwnd, 0, 0,
                                INITIAL_WIDTH + self.offset[0],
                                scale_height + self.offset[1] - 1, True)

    """
    Resets the x and y offset of an image and blits it
    """
    def reset_image(self):
        self.ypos = 0
        self.xpos = 0
        self.blit_image()

    def blit_textbox(self, text):
        font = pygame.font.SysFont(STATUS_FONT, STATUS_FONT_SIZE)
        lines = [font.render(l, 1, (0,0,0)) for l in text]
        width = max([l.get_rect().width for l in lines]) + 20
        height = sum([l.get_rect().height for l in lines]) + 20
        left = self.GetClientRect().centerx-width/2
        top = self.GetClientRect().centery-height/2
        # Draw rectangle
        rect = (left, top,width,height)
        self.screen.fill((255,255,255), rect)
        pygame.draw.rect(self.screen, 0, rect, 2)

        for i in range(len(lines)):
            self.screen.blit(lines[i],
                    (left+10, top+10+i*font.get_height(), width, height))
    """
    Displays the help splash screen
    """
    def blit_help(self):
        # Draw text
        text = ["Welcome to Comic Viewer!","","Shortcut keys:",
                "  Shift: Show page number","  Page Down/Up: Flip pages",
                "  w: Fullscreen",
                      "  Scroll wheel: Navigate page and flip pages",
                "  Arrow keys: Navigate page and flip pages",
                "  t: View two pages at once","",
                "Press any key to close"]
        self.blit_textbox(text)
                

    """
    Displays text of the cur / max page number in the bottomright
    Crappy support for non-Windows, only works well fullscreen
    """
    def blit_stats(self):
        end = len(self.filecacher)
        cur = self.filecacher.idx + 1

        font = pygame.font.SysFont(STATUS_FONT, STATUS_FONT_SIZE)
        page_num = font.render("%d / %d" % (cur,end), 1, (0,0,0), (255,255,255))
        line_bot = page_num.get_rect()
        line_bot.bottomright = self.GetClientRect().bottomright

        status_text = os.path.basename(self.filecacher.filename)
        status_line = font.render(status_text, True, (0,0,0), (255,255,255))
        line_top = status_line.get_rect()
        line_top.bottomright = line_bot.topright
        self.screen.blit(page_num, line_bot)
        self.screen.blit(status_line, line_top)
        
    def blit_stats_temporary(self):
        self.blit_stats()
        self.ticks_till_blit = TICKS_FOR_STATS
    
    def scale_image(self, image, scale_sz):
        scale_sz = [int(i) for i in scale_sz]
        if image.get_bitsize() == 8:
            image = image.convert(24)
        try:
            image = pygame.transform.smoothscale(image, scale_sz)
        except ValueError:
            image = pygame.transform.scale(image, scale_sz)
        return image
    

    """
    Blits the current image onto the screen accounting for x and y offset,
    along with scaling and centering
    """
    def blit_image(self):
        x = self.xpos
        y = self.ypos
        self.image = self.filecacher.load_image_relative()
        client_rect = self.GetClientRect()
        client_sz = client_rect.size
        
        width = self.image.get_size()[0]
        if self.twoup:
            self.image2 = self.filecacher.load_image_relative(skip=1)
            width += self.image2.get_size()[0]
        
        # Fit oversized widths to client width
        if width > client_sz[0]:
            scale_factor = float(client_sz[0]) / width
            scale_sz1 = (self.image.get_size()[0] * scale_factor, self.image.get_size()[1] * scale_factor)
            self.image = self.scale_image(self.image, scale_sz1)
            if self.twoup:
                scale_sz2 = (self.image2.get_size()[0] * scale_factor, self.image2.get_size()[1] * scale_factor)
                self.image2 = self.scale_image(self.image2, scale_sz2)

        self.screen.fill(0) # fill screen w/ black
        
        img_rect = self.image.get_rect()
        img_rect.move_ip(x, y) # shift image according to scroll
        if self.twoup:
            img_rect2 = self.image.get_rect()
            img_rect2.move_ip(x, y)
        # performs an x-center only if fullscreen on non-Windows OS
        # On Windows, however, always centers
        if self.fullscreen or os.name == 'nt':
            img_rect.centerx = client_rect.centerx
            if self.twoup:
                img_rect.centerx -= img_rect.width/2
                img_rect2.centerx = client_rect.centerx + img_rect2.width/2
                
        self.screen.blit(self.image, img_rect) # blit image to specific coordinates
        if self.twoup:
            self.screen.blit(self.image2, img_rect2)
        img_size = self.image.get_size()
        client_size = self.GetClientRect().size
        # If we are not at the bottom and the image is bigger than the client
        #if self.ypos != client_size[1] - img_size[1] and img_size[1] > client_size[1]:
            #self.screen.fill(pygame.Color('green'), rect=((client_size[0]+img_size[0])/2,client_size[1]-10,10,10))
        if self.ypos == client_size[1] - img_size[1] and img_size[1] > client_size[1]:
            self.screen.fill(pygame.Color('green'), rect=(0,client_size[1]-1,client_size[0],1))
            
        if self.ticks_till_blit:
            self.blit_stats()

    """
    Switches to a window by one of two methods
    On Windows, it switches back to a RESIZABLE window from the NOFRAME and then
    moves the window back into its previous position
    On other OS, it recreates the window using the default size and position
    """
    def create_window(self, size=None):
        if size == None:
            size = self.size
        if os.name == 'nt':
            self.screen = pygame.display.set_mode(
                self.desktop_size, pygame.RESIZABLE, 32)
            hwnd = pygame.display.get_wm_info()['window']
            win32gui.MoveWindow(hwnd, size[0], size[1], size[2], size[3], True)
        else:
            pygame.display.quit()
            os.environ['SDL_VIDEO_WINDOW_POS'] = ""
            self.screen = pygame.display.set_mode(
                self.desktop_size, pygame.RESIZABLE, 32)
        self.reset_image()
        pygame.mouse.set_visible(True)

    """
    Switches to fullscreen by one of two methods
    On Windows, it resizes the window and removes the border
    On other OS, it quits the pygame display and creates a new SDL display
      at location 0,0 using an environmental variable
    """
    def create_fullscreen(self):
        pygame.mouse.set_visible(False)
        if os.name == 'nt':
            # store last size for when returning to window mode
            sz = self.GetWindowRect()
            self.size = (sz[0], sz[1], sz[2]-sz[0], sz[3]-sz[1])
            self.screen = pygame.display.set_mode(self.desktop_size,
                                                  pygame.NOFRAME)
            hwnd = pygame.display.get_wm_info()['window']
            win32gui.MoveWindow(hwnd, 0, 0, self.desktop_size[0],
                                self.desktop_size[1], True)
        else:
            pygame.display.quit()
            os.environ['SDL_VIDEO_WINDOW_POS'] = "0,0"
            self.screen = pygame.display.set_mode(self.desktop_size,
                                                  pygame.NOFRAME)
        self.reset_image()

    def minimize_window(self):
        pygame.display.iconify()

    def shift_image(self, x=0, y=0):
        img_sz = self.image.get_size()
        sz = self.GetClientRect().size
        self.xpos += x
        self.ypos += y
        if self.ypos > 0: # should only be negative numbers
            self.ypos = 0
        # don't go past the bottom
        if sz[1] - self.ypos >= img_sz[1]:
            self.ypos = sz[1] - img_sz[1]
        # If the scaled image fits entirely on the client,
        # then do not shift at all
        if img_sz[1] < sz[1]:
            self.ypos = 0
        self.blit_image()

    # Scroll height is added to image's position to move image on the client
    @staticmethod
    def calc_scroll_height(img_height, client_height, percent):
        ceil = lambda x: int(x) + 1 if x - int(x) > 0 else int(x)
        scroll_height = percent * client_height
        # Calculate the difference between the client ht and img ht
        # This is the portion of the image that is off screen
        offscreen_height = float(img_height - client_height)
        # Calc a number of scroll points based on the portion of the image off
        # screen and the percentage-based scroll height
        scroll_pts = ceil(offscreen_height / int(scroll_height))
        # Finally return the scroll height, which is the portion of the image
        # off screen divided by how many scroll points there are
        return offscreen_height / scroll_pts

    def scroll_image_up(self, percent=0.4):
        if self.ypos == 0: # Scrolling up while at the top of an image
            self.prev_image()
            return
        img_sz = self.image.get_size()
        client_sz = self.GetClientRect().size
        scroll_height = self.calc_scroll_height(img_sz[1], client_sz[1], percent)
        self.shift_image(y=scroll_height)

    def scroll_image_down(self, percent=0.4):
        img_sz = self.image.get_size()
        client_sz = self.GetClientRect().size
        # If at the bottom of the image OR
        # the image fits entirely on the client
        # then go to the next page
        if self.ypos == client_sz[1] - img_sz[1] or img_sz[1] < client_sz[1]:
            # If on the last image and at the bottom
            # then just show stats and don't move
            if self.filecacher.at_end():
                self.blit_stats_temporary()
                return
            self.next_image()
            return
        scroll_height = self.calc_scroll_height(img_sz[1], client_sz[1], percent)
        self.shift_image(y=-scroll_height)

    def browse_new_archive(self):
        filename = None
        if os.name == 'nt':
            try:
                filename,_,_ = win32gui.GetOpenFileNameW(
                    Filter="Comic archives (*.cbz *.cbr)\0*.cbr;*.cbz\0All Files (*.*)\0*.*\0")
            except Exception:
                pass
            hwnd = pygame.display.get_wm_info()['window']
            win32gui.SetFocus(hwnd)
        else:
            root = Tkinter.Tk()
            root.withdraw()
            filename = tkFileDialog.askopenfilename()
        if not filename:
            return
        self.load_archive(filename)
        
    def load_archive(self, filename):
        self.filecacher.load_archive(filename)
        pygame.display.set_caption(os.path.basename(filename))
        self.image = self.filecacher.goto_home()
        self.reset_image()
        
    def load_next_archive(self):
        """ Loads the next archive in the current path """
        current_dir = os.path.dirname(self.filecacher.filename)
        files = os.listdir(current_dir)
        files = [f for f in files if re.match('^.*(cbr|cbz|rar|zip)$', f)]
        idx = files.index(os.path.basename(self.filecacher.filename))
        if idx + 1 >= len(files):
            self.blit_textbox(["This is the last archive in the folder."])
            return
        self.load_archive(os.path.join(current_dir, files[idx+1]))
        
    def next_image(self):
        skip = 2 if self.twoup else 1
        self.image = self.filecacher.progress_image(True, skip)
        self.reset_image()
        self.blit_stats_temporary()
        
    def prev_image(self):
        skip = 2 if self.twoup else 1
        home = self.filecacher.at_home() # needs to be before we progress_image
        self.image = self.filecacher.progress_image(False, skip)
        self.xpos = 0
        self.ypos = 0
        if not home:
            self.shift_image(y=-99999) # this calls blit_image()
        else:
            self.blit_image()
        self.blit_stats_temporary()
        
    def progress_image_max(self, end=True):
        """ Goes to the home or end """
        if end:
            self.image = self.filecacher.goto_end()
        else:
            self.image = self.filecacher.goto_home()
        self.reset_image()
        self.blit_stats_temporary()

    """
    Handles key presses and starts preload threads
    """
    def run(self):
        moving = None
        ticks_held = 0
        fast_movement = False

        self.filecacher.preload_in_thread()
        
        self.blit_image()
        self.blit_help()
        
        # Initial event loop to display the help
        show_help = True
        while show_help:
            self.clock.tick(50)
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    return
                if event.type in [pygame.KEYDOWN, pygame.MOUSEBUTTONDOWN]:
                    show_help = False
            pygame.display.flip()
            
        self.blit_image()
        
        while True:
            self.clock.tick(50)
            
            # This is used to leave the stats up for a short time before
            # removing them
            if self.ticks_till_blit is not None:
                self.ticks_till_blit -= 1
                if self.ticks_till_blit < 0:
                    self.ticks_till_blit = None
                    self.blit_image()

            #  moving is the current action being performed
            if moving is not None:
                ticks_held += 1
            # ticks_held is the number of ticks the button is held for
            #fast_movement is reached after the button has been held for a while
            if ticks_held > 20:
                fast_movement = True
            
            # In fast_movement, every 3 ticks the action is performed
            if fast_movement and ticks_held > 3:
                ticks_held = 0
                moving()
                
            """
            # Alternative behavior:
            if any([pygame.key.get_pressed()[k] for k in [pygame.K_RSHIFT,
                                                          pygame.K_LSHIFT]]):
                self.blit_stats_temporary()"""

            ##################
            # Event Handling #
            ##################
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    return
                elif event.type == pygame.KEYDOWN:
                    key = event.key
                    if key in [pygame.K_PAGEDOWN, pygame.K_RIGHT]:
                        moving = self.next_image
                        moving()
                    elif key in [pygame.K_PAGEUP, pygame.K_LEFT]:
                        moving = self.prev_image
                        moving()
                    elif key == pygame.K_DOWN:
                        moving = self.scroll_image_down
                        moving()
                    elif key == pygame.K_UP:
                        moving = self.scroll_image_up
                        moving()
                    elif key == pygame.K_HOME:
                        self.progress_image_max(end=False)
                    elif key == pygame.K_END:
                        self.progress_image_max(end=True)
                    elif key in [pygame.K_LSHIFT, pygame.K_RSHIFT]:
                        self.blit_stats()
                    elif key == pygame.K_w:
                        if self.fullscreen:
                            self.create_window()
                        else:
                            self.create_fullscreen()
                        self.fullscreen ^= True
                    elif key == pygame.K_t:
                        self.twoup ^= True
                        self.reset_image()
                    elif key == pygame.K_ESCAPE:
                        quit()
                        return
                    elif key == pygame.K_m:
                        self.minimize_window()
                    elif key == pygame.K_o:
                        self.browse_new_archive()
                    elif key == pygame.K_l:
                        self.load_next_archive()
                elif event.type == pygame.KEYUP and moving is not None:
                    # Fast movement finished
                    # after a fast_movement it's best to preload again
                    self.filecacher.preload_in_thread()
                    ticks_held = 0
                    moving = None
                    fast_movement = False
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 4: # scrollwheel up
                        self.scroll_image_up(percent=0.2)
                    elif event.button == 5: # scrollwheel down
                        self.scroll_image_down(percent=0.2)
                elif event.type == pygame.VIDEORESIZE:
                    # New window size means the image needs to be rescaled
                    self.blit_image()

            pygame.display.flip()

if __name__ == "__main__":
    cv = ComicViewer(sys.argv[1])
    cv.run()
