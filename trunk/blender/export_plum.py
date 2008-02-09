#!BPY

"""
Name: 'Plutocracy Model (.plum)...'
Blender: 245
Group: 'Export'
Tooltip: 'Save a Plutocracy Model File'
"""

__author__ = "Michael Levin, Campbell Barton, Jiri Hnidek"
__url__ = ['www.blender.org', 'blenderartists.org']
__version__ = "1.0"

__bpydoc__ = """\
This script is an exporter to Plutocracy Model file format.
(Based on the OBJ exporter by Campbell Barton and Jiri Hnidek.)

Usage:

Select the objects you wish to export and run this script from "File->Export" menu.
Selecting the default options from the popup box will be good in most cases.
All objects that can be represented as a mesh (mesh, curve, metaball, surface, text3d)
will be exported as mesh data.
"""


# --------------------------------------------------------------------------
# Plutocracy Model Exporter by Michael "risujin" Levin
# based on OBJ Export v1.1 by Campbell Barton (AKA Ideasman)
# --------------------------------------------------------------------------
# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****
# --------------------------------------------------------------------------


import Blender
from Blender import Mesh, Scene, Window, sys, Image, Draw
import BPyMesh
import BPyObject
import BPySys
import BPyMessages


def vec_eq(a, b):
        for i in range(0, len(a)):
                if a[i] != b[i]:
                        return False
        return True


def sanitize(s):
        return s.replace('"', '\\"')


def sanitize_strip(s):
        if s[0:2] == '//':
                s = s[2:len(s)]
        while s[0:3] == '../':
                s = s[3:len(s)]
        return sanitize(s)


def get_image_filename(scn, me):
        for mat in me.materials:
                for mtex in mat.getTextures():
                        if not mtex:
                                continue
                        image = mtex.tex.getImage()
                        if image and image.getFilename():
                                return image.getFilename()
        return ''


def write_frame(scn, file, objects):

        # Blender has front and side confused
        mat_xrot90 = Blender.Mathutils.RotationMatrix(-90, 4, 'x')

        # Get all meshes
        for me, ob, ob_mat in objects:
                me.transform(ob_mat * mat_xrot90)

                # Indicate that this is a new object
                file.write('\no\n')
                verts = [ ]
                for f in me.faces:
                        inds = [ 0, 0, 0 ]
                        reused = False
                        if len(f.v) != 3:
                                print 'WARNING: non-triangular face'
                        for j in range(0, 3):
                                vec = [ f.v[j].co.x, f.v[j].co.y, \
                                        f.v[j].co.z,  f.no.x, f.no.y, \
                                        f.no.z, 0, 0 ]
                                if f.smooth:
                                        vec[3] = f.v[j].no[0]
                                        vec[4] = f.v[j].no[1]
                                        vec[5] = f.v[j].no[2]
                                if me.faceUV:
                                        vec[6] = f.uv[j].x
                                        vec[7] = 1 - f.uv[j].y # v flipped!

                                # See if this vertex has already been written
                                for i in range(0, len(verts)):
                                        if vec_eq(verts[i], vec):
                                                inds[j] = i
                                                reused = True
                                                break
                                else:
                                        inds[j] = len(verts)
                                        verts.append(vec)
                                        file.write('v %g %g %g %g %g %g %g %g\n'
                                                   % tuple(vec))
                        if reused:
                                file.write('i %d %d %d\n' % tuple(inds))


def write(filename):
        if not filename.lower().endswith('.plum'):
                filename += '.plum'
        if not BPyMessages.Warning_SaveOver(filename):
                return

        Window.EditMode(0)
        Window.WaitCursor(1)

        # Export an animation up to and including the current frame
        scn = orig_scene = Scene.GetCurrent()
        orig_frame = Blender.Get('curframe')

        # Extract the existing 'anims:' block
        # TODO: we can use this as an upper limit on the animation frames
        anims = ''
        max_frame = orig_frame
        try:
                file = open(filename, 'r')
                parsing_anim = False
                line = '\n'
                print "Parsing old '%s'" % filename
                while len(line) > 0 and line[-1] == '\n':
                        line = file.readline()
                        if parsing_anim:
                                anims += line;
                                end_pos = line.find('end')
                                comment_pos = line.find('#')
                                if comment_pos == -1:
                                        comment_pos = 9999;
                                if end_pos >= 0 and end_pos < comment_pos:
                                        break
                                sline = line;
                                if comment_pos < 9999:
                                        sline = sline[0:comment_pos]
                                sline = sline.split();
                                if len(sline) >= 4 and sline[-3] > max_frame:
                                        max_frame = int(sline[-3])
                                continue
                        anim_pos = line.find('anims:')
                        comment_pos = line.find('#')
                        if comment_pos == -1:
                                comment_pos = 9999;
                        if anim_pos >= 0 and anim_pos < comment_pos:
                                parsing_anim = True
                                anims += line;
                file.close()
        except:
                pass
        if len(anims) < 1:
                anims = 'anims:\n        "idle" 1 %d 30 "idle"\nend\n' % \
                        orig_frame;

        file = open(filename, "w")
        file.write(('########################################' + \
                    '########################################\n' + \
                    '# Blender3D v%s PLUM file: %s\n' + \
                    '# www.blender3d.org\n' + \
                    '########################################' + \
                    '########################################\n\n') % \
                    (Blender.Get('version'), \
                   Blender.Get('filename').split('/')[-1].split('\\')[-1] ))

        # Animation definitions
        file.write('# [name] [start] [end] [fps] [end-anim]\n' + anims)

        # Make an array of all meshes in the scene
        objects = [ ]
        for ob in scn.objects:
                if ob.getType() != 'Mesh':
                        print "Skipping non-mesh object '%s'" % ob.name
                        continue
                me = ob.getData(False, True)
                if not me or not me.faces or \
                   not (len(me.faces) + len(me.verts)):
                        continue
                objects.append((me, ob, ob.mat))

        # List all the objects ahead of time
        file.write('\n# object [name] [material]\n')
        for me, ob, ob_mat in objects:
                file.write('object "%s" "%s"\n' % \
                           (sanitize(ob.name), \
                            sanitize_strip(get_image_filename(scn, me))))

        # Write all frames into one file
        scene_frames = range(1, max_frame + 1)
        print "Exporting '%s', %d frames:" % (filename, max_frame + 1)
        for frame in scene_frames:
                print "... frame %d" % frame
                file.write(('\n########################################' + \
                            '########################################\n' + \
                            'frame %d\n' + \
                            '########################################' + \
                            '########################################\n') % \
                            frame);
                Blender.Set('curframe', frame)
                write_frame(scn, file, objects)

        print "Done"
        file.close()

        Blender.Set('curframe', orig_frame)

if __name__ == '__main__':
        Window.FileSelector(write, 'Export Plutocracy PLUM', \
                            sys.makename(ext='.plum'))

