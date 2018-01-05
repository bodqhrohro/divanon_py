from sklearn.neural_network import MLPClassifier

class MLP:
    def __init__(self, verbose = True):
        self.classifier = MLPClassifier(
                hidden_layer_sizes = (10),
                activation = 'logistic',
                solver = 'sgd',
                learning_rate = 'constant',
                learning_rate_init = 0.7,
                max_iter = 10000,
                tol = 0.00001,
                momentum = 0,
                verbose = verbose,
            )

    def train(self, X, y):
        self.classifier.fit(X, y)

    def estimate(self, X):
        return self.classifier.predict(X)
