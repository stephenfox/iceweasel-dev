__VARS := $(.VARIABLES)

dump:
	@$(foreach var,$(sort $(filter-out $(__VARS) __VARS preprocess,$(.VARIABLES))),echo '$(var) = $(subst ','\'',$(subst \,\\,$($(var))))';)

dump-%:
	@echo $($*)

.PHONY: dump
