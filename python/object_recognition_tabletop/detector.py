#!/usr/bin/env python
"""
Module defining the transparent objects detector to find objects in a scene
"""

from ecto import BlackBoxCellInfo as CellInfo, BlackBoxForward as Forward
from object_recognition_core.db import ObjectDb, ObjectDbParameters
from object_recognition_core.pipelines.detection import DetectorBase
from object_recognition_tabletop.ecto_cells.tabletop_table import TablePose, Clusterer, TableDetector
import ecto

class TabletopTableDetector(ecto.BlackBox, DetectorBase):
    def __init__(self, *args, **kwargs):
        ecto.BlackBox.__init__(self, *args, **kwargs)
        DetectorBase.__init__(self)

    @classmethod
    def declare_cells(cls, _p):
        return {'passthrough': ecto.PassthroughN(items={'K': 'The original calibration matrix',
                                                        'points3d': 'The 3d points as cv::Mat_<cv::Vec3f>.'}),
                'table_detector': TableDetector(),
                'table_pose': CellInfo(TablePose),
                'clusterer': Clusterer()
                }

    def declare_forwards(self, p):
        p = {'clusterer': 'all', 'table_detector': 'all'}

        i = {'passthrough': 'all'}

        o = {'table_detector': [Forward('clouds_hull')],
             'clusterer': [Forward('clusters')],
             'table_pose': [Forward('pose_results')]
             }

        return (p,i,o)

    def connections(self, _p):
        # First find the table, then the pose
        connections = [ self.passthrough['points3d', 'K'] >> self.table_detector['points3d', 'K'],
                        self.table_detector['table_coefficients'] >> self.table_pose['table_coefficients'] ]
        # also find the clusters of points
        connections += [ self.passthrough['points3d'] >> self.clusterer['points3d'],
                         self.table_detector['table_coefficients'] >> self.clusterer['table_coefficients'],
                         self.table_detector['table_mask'] >> self.clusterer['table_mask'] ]

        return connections

########################################################################################################################

class TabletopObjectDetector(ecto.BlackBox, DetectorBase):

    def __init__(self, *args, **kwargs):
        self._params = kwargs
        ecto.BlackBox.__init__(self, *args, **kwargs)
        DetectorBase.__init__(self)

    def declare_cells(self, _p):
        from object_recognition_tabletop.ecto_cells import tabletop_object

        return {'main': tabletop_object.ObjectRecognizer(object_ids=self._params['object_ids'],
                                                         db=ObjectDb(ObjectDbParameters(self._params['db'])))}

    def declare_forwards(self, p):
        return ({},{'main':'all'},{'main':'all'})

    def connections(self, _p):
        return [self.main]
