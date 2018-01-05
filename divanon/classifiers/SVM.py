from sklearn import svm

class SVM:
    def __init__(self, verbose = True):
        self.classifier = svm.SVC(
                kernel = 'linear',
                C = 1,
                cache_size = 100,
                shrinking = True,
                verbose = verbose,
            )

    def train(self, X, y):
        self.classifier.fit(X, y)

    def estimate(self, X):
        return self.classifier.predict(X)
