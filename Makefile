tests = $(shell ls tests/*.py)
test:
	@$(foreach test,$(tests),PYTHONPATH=:.. python3 $(test);)
