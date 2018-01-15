# from divanon.vendor.cascade_net import CascadeNet, logistic, d_logistic
from divanon.dblink.cmu_wrapper import CMUWrapper
import numpy as np


class CCN:
    def __init__(self, name: str, verbose=True):
        self.filename = name
        self.full_filename = name + '.data'
        self.classifier = CMUWrapper()
        self.verbose = verbose

    def train(self, X, y):
        self.classifier.generateTrainSet(
            self.full_filename, X, y, np.max(y) + 1
        )

    def estimate(self, X):
        return self.classifier.estimateByTrainFile(
            self.full_filename,
            X,
            self.verbose
        )
