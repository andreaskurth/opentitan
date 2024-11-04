set COMPONENT prim_reg_cdc
set MANUALLY_ANALYZE_INTENT true

analyze -F scratch/[exec git rev-parse --abbrev-ref HEAD]/chip_earlgrey_asic-cdc-meridiancdc/default/syn-icarus/lowrisc_systems_chip_earlgrey_asic_0.1.scr

elaborate $COMPONENT

if {$MANUALLY_ANALYZE_INTENT} {
  analyze_intent -output $COMPONENT.intent.env
} else {
  read_env $COMPONENT.intent.env
  analyze_intent
}

verify_cdc -write_cdc_db $COMPONENT
