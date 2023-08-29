//Set up registers and a basic memory map for SC144xx (currently only SC14481)
//@author fridtjof
//@category Project
//@keybinding 
//@menupath 
//@toolbar 

import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.*;
import ghidra.program.model.lang.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.util.*;
import ghidra.program.model.reloc.*;
import ghidra.program.model.data.*;
import ghidra.program.model.block.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.scalar.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;

import ghidra.app.util.importer.MessageLog;
import ghidra.program.model.listing.Program;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.util.CodeUnitInsertionException;
import ghidra.util.exception.InvalidInputException;

import static ghidra.app.util.MemoryBlockUtils.createUninitializedBlock;
import static ghidra.program.model.data.DataUtilities.createData;

public class DialogSc144xxRegs extends GhidraScript {
    public void run() throws Exception {
        var log = new MessageLog();

        addHardwareBlocks(this.currentProgram, log);
        populateHardwareBlocks(this.currentProgram);
    }

    final class DataTypes {
        public static final ByteDataType u8 = ByteDataType.dataType;
        public static final WordDataType u16 = WordDataType.dataType;
        public static final DWordDataType u32 = DWordDataType.dataType;
        public static final CharDataType ch = CharDataType.dataType;

        //static Array array(DataType d, int size) {
        //    return new ArrayDataType(d, size, -1);
        //}
    }

    public static void addHardwareBlocks(Program program, MessageLog log) {
        var source = "SC14481 Hardware";
        var as = program.getAddressFactory().getDefaultAddressSpace();
        //createUninitializedBlock(program, false, "bootrom", as.getAddress(0xFE_F000), 0xFE_F800 - 0xFE_F000, "Boot ROM", source, true, true, false, log);
        
        var io = createUninitializedBlock(program, false, "sfr", as.getAddress(0xFF_0000), 0xFF_FC00 - 0xFF_0000, "Special Function Registers", "", true, true, false, log);
        if (io != null) {
            io.setVolatile(true);
        }
    }

    record Reg(String name, long address, DataType type) {}

    public static void populateHardwareBlocks(Program program) throws CodeUnitInsertionException, InvalidInputException {
        var as = program.getAddressFactory().getDefaultAddressSpace();

        Reg[] regsSC14481 = {
            new Reg("CLK_AMBA_REG", 0xFF_4000, DataTypes.u16),
            new Reg("CLK_CODEC_DIV_REG", 0xFF_4002, DataTypes.u16),
            new Reg("CLK_CODEC_REG", 0xFF_4004, DataTypes.u16),
            new Reg("CLK_DSP_REG", 0xFF_4006, DataTypes.u16),
            new Reg("CLK_FREQ_TRIM_REG", 0xFF_400A, DataTypes.u16),
            new Reg("CLK_PER_DIV_REG", 0xFF_400C, DataTypes.u16),
            new Reg("CLK_PER10_DIV_REG", 0xFF_400E, DataTypes.u16),
            new Reg("CLK_PLL_CTRL_REG", 0xFF_4010, DataTypes.u16),
            new Reg("CLK_PLL_DIV_REG", 0xFF_4012, DataTypes.u16),
            new Reg("CLK_XTAL_CTRL_REG", 0xFF_4018, DataTypes.u16),
            new Reg("CLK_AUX_REG", 0xFF_401A, DataTypes.u16),
            new Reg("CLK_PCM_DIV_REG", 0xFF_401C, DataTypes.u16),
            new Reg("CLK_ULP_CTRL_REG", 0xFF_401E, DataTypes.u16),
            new Reg("DMA0_A_STARTL_REG", 0xFF_4400, DataTypes.u16),
            new Reg("DMA0_A_STARTH_REG", 0xFF_4402, DataTypes.u16),
            new Reg("DMA0_B_STARTL_REG", 0xFF_4404, DataTypes.u16),
            new Reg("DMA0_B_STARTH_REG", 0xFF_4406, DataTypes.u16),
            new Reg("DMA0_INT_REG", 0xFF_4408, DataTypes.u16),
            new Reg("DMA0_LEN_REG", 0xFF_440A, DataTypes.u16),
            new Reg("DMA0_CTRL_REG", 0xFF_440C, DataTypes.u16),
            new Reg("DMA0_IDX_REG", 0xFF_440E, DataTypes.u16),
            new Reg("DMA1_A_STARTL_REG", 0xFF_4410, DataTypes.u16),
            new Reg("DMA1_A_STARTH_REG", 0xFF_4412, DataTypes.u16),
            new Reg("DMA1_B_STARTL_REG", 0xFF_4414, DataTypes.u16),
            new Reg("DMA1_B_STARTH_REG", 0xFF_4416, DataTypes.u16),
            new Reg("DMA1_INT_REG", 0xFF_4418, DataTypes.u16),
            new Reg("DMA1_LEN_REG", 0xFF_441A, DataTypes.u16),
            new Reg("DMA1_CTRL_REG", 0xFF_441C, DataTypes.u16),
            new Reg("DMA1_IDX_REG", 0xFF_441E, DataTypes.u16),
            new Reg("DMA2_A_STARTL_REG", 0xFF_4420, DataTypes.u16),
            new Reg("DMA2_A_STARTH_REG", 0xFF_4422, DataTypes.u16),
            new Reg("DMA2_B_STARTL_REG", 0xFF_4424, DataTypes.u16),
            new Reg("DMA2_B_STARTH_REG", 0xFF_4426, DataTypes.u16),
            new Reg("DMA2_INT_REG", 0xFF_4428, DataTypes.u16),
            new Reg("DMA2_LEN_REG", 0xFF_442A, DataTypes.u16),
            new Reg("DMA2_CTRL_REG", 0xFF_442C, DataTypes.u16),
            new Reg("DMA2_IDX_REG", 0xFF_442E, DataTypes.u16),
            new Reg("DMA3_A_STARTL_REG", 0xFF_4430, DataTypes.u16),
            new Reg("DMA3_A_STARTH_REG", 0xFF_4432, DataTypes.u16),
            new Reg("DMA3_B_STARTL_REG", 0xFF_4434, DataTypes.u16),
            new Reg("DMA3_B_STARTH_REG", 0xFF_4436, DataTypes.u16),
            new Reg("DMA3_INT_REG", 0xFF_4438, DataTypes.u16),
            new Reg("DMA3_LEN_REG", 0xFF_443A, DataTypes.u16),
            new Reg("DMA3_CTRL_REG", 0xFF_443C, DataTypes.u16),
            new Reg("DMA3_IDX_REG", 0xFF_443E, DataTypes.u16),
            new Reg("TEST_ENV_REG", 0xFF_4800, DataTypes.u16),
            new Reg("TEST_CTRL_REG", 0xFF_4802, DataTypes.u16),
            new Reg("TEST_CTRL2_REG", 0xFF_4804, DataTypes.u16),
            new Reg("TEST_CTRL3_REG", 0xFF_4806, DataTypes.u16),
            new Reg("BANDGAP_REG", 0xFF_4810, DataTypes.u16),
            new Reg("BAT_CTRL_REG", 0xFF_4812, DataTypes.u16),
            new Reg("BAT_CTRL2_REG", 0xFF_4814, DataTypes.u16),
            new Reg("BAT_STATUS_REG", 0xFF_4816, DataTypes.u16),
            new Reg("BAT_SOC_HIGH_REG", 0xFF_4818, DataTypes.u16),
            new Reg("BAT_SOC_LOW_REG", 0xFF_481A, DataTypes.u16),
            new Reg("CP_CTRL_REG", 0xFF_481C, DataTypes.u16),
            new Reg("PAD_CTRL_REG", 0xFF_481E, DataTypes.u16),
            new Reg("LED_CTRL_REG", 0xFF_4820, DataTypes.u16),
            new Reg("CP_CTRL2_REG", 0xFF_4822, DataTypes.u16),
            new Reg("P0_DATA_REG", 0xFF_4830, DataTypes.u16),
            new Reg("P0_SET_DATA_REG", 0xFF_4832, DataTypes.u16),
            new Reg("P0_RESET_DATA_REG", 0xFF_4834, DataTypes.u16),
            new Reg("P0_DIR_REG", 0xFF_4836, DataTypes.u16),
            new Reg("P0_MODE_REG", 0xFF_4838, DataTypes.u16),
            new Reg("P1_DATA_REG", 0xFF_4840, DataTypes.u16),
            new Reg("P1_SET_DATA_REG", 0xFF_4842, DataTypes.u16),
            new Reg("P1_RESET_DATA_REG", 0xFF_4844, DataTypes.u16),
            new Reg("P1_DIR_REG", 0xFF_4846, DataTypes.u16),
            new Reg("P1_MODE_REG", 0xFF_4848, DataTypes.u16),
            new Reg("P2_DATA_REG", 0xFF_4850, DataTypes.u16),
            new Reg("P2_SET_DATA_REG", 0xFF_4852, DataTypes.u16),
            new Reg("P2_RESET_DATA_REG", 0xFF_4854, DataTypes.u16),
            new Reg("P2_DIR_REG", 0xFF_4856, DataTypes.u16),
            new Reg("P2_MODE_REG", 0xFF_4858, DataTypes.u16),
            new Reg("P3_DATA_REG", 0xFF_4860, DataTypes.u16),
            new Reg("P3_SET_DATA_REG", 0xFF_4862, DataTypes.u16),
            new Reg("P3_RESET_DATA_REG", 0xFF_4864, DataTypes.u16),
            new Reg("P3_DIR_REG", 0xFF_4866, DataTypes.u16),
            new Reg("P3_MODE_REG", 0xFF_4868, DataTypes.u16),
            new Reg("P4_DATA_REG", 0xFF_4870, DataTypes.u16),
            new Reg("P4_SET_DATA_REG", 0xFF_4872, DataTypes.u16),
            new Reg("P4_RESET_DATA_REG", 0xFF_4874, DataTypes.u16),
            new Reg("P4_DIR_REG", 0xFF_4876, DataTypes.u16),
            new Reg("P4_MODE_REG", 0xFF_4878, DataTypes.u16),
            new Reg("P5_DATA_REG", 0xFF_4880, DataTypes.u16),
            new Reg("P5_SET_DATA_REG", 0xFF_4882, DataTypes.u16),
            new Reg("P5_RESET_DATA_REG", 0xFF_4884, DataTypes.u16),
            new Reg("P5_DIR_REG", 0xFF_4886, DataTypes.u16),
            new Reg("P5_MODE_REG", 0xFF_4888, DataTypes.u16),
            new Reg("PORT_TRACE_CTRL_REG", 0xFF_48E0, DataTypes.u16),
            new Reg("UART_CTRL_REG", 0xFF_4900, DataTypes.u16),
            new Reg("UART_RX_TX_REG", 0xFF_4902, DataTypes.u16),
            new Reg("UART_CLEAR_TX_INT_REG", 0xFF_4904, DataTypes.u16),
            new Reg("UART_CLEAR_RX_INT_REG", 0xFF_4906, DataTypes.u16),
            new Reg("UART_ERROR_REG", 0xFF_4908, DataTypes.u16),
            new Reg("UART2_CTRL_REG", 0xFF_4910, DataTypes.u16),
            new Reg("UART2_RX_TX_REG", 0xFF_4912, DataTypes.u16),
            new Reg("UART2_CLEAR_TX_INT_REG", 0xFF_4914, DataTypes.u16),
            new Reg("UART2_CLEAR_RX_INT_REG", 0xFF_4916, DataTypes.u16),
            new Reg("UART2_ERROR_REG", 0xFF_4918, DataTypes.u16),
            new Reg("ACCESS1_IN_OUT_REG", 0xFF_4920, DataTypes.u16),
            new Reg("ACCESS1_CTRL_REG", 0xFF_4922, DataTypes.u16),
            new Reg("ACCESS1_CLEAR_INT_REG", 0xFF_4924, DataTypes.u16),
            new Reg("ACCESS2_IN_OUT_REG", 0xFF_4930, DataTypes.u16),
            new Reg("ACCESS2_CTRL_REG", 0xFF_4932, DataTypes.u16),
            new Reg("ACCESS2_CLEAR_INT_REG", 0xFF_4934, DataTypes.u16),
            new Reg("SPI_CTRL_REG", 0xFF_4940, DataTypes.u16),
            new Reg("SPI_RX_TX_REG0", 0xFF_4942, DataTypes.u16),
            new Reg("SPI_RX_TX_REG1", 0xFF_4944, DataTypes.u16),
            new Reg("SPI_CLEAR_INT_REG", 0xFF_4946, DataTypes.u16),
            new Reg("SPI2_CTRL_REG", 0xFF_4950, DataTypes.u16),
            new Reg("SPI2_CLEAR_INT_REG", 0xFF_4956, DataTypes.u16),
            new Reg("ADC_CTRL_REG", 0xFF_4960, DataTypes.u16),
            new Reg("ADC_CTRL1_REG", 0xFF_4962, DataTypes.u16),
            new Reg("ADC_CLEAR_INT_REG", 0xFF_4964, DataTypes.u16),
            new Reg("ADC0_REG", 0xFF_4966, DataTypes.u16),
            new Reg("ADC1_REG", 0xFF_4968, DataTypes.u16),
            new Reg("TIMER_CTRL_REG", 0xFF_4970, DataTypes.u16),
            new Reg("TIMER0_ON_REG", 0xFF_4972, DataTypes.u16),
            new Reg("TIMER0_RELOAD_M_REG", 0xFF_4974, DataTypes.u16),
            new Reg("TIMER0_RELOAD_N_REG", 0xFF_4976, DataTypes.u16),
            new Reg("TIMER1_RELOAD_M_REG", 0xFF_4978, DataTypes.u16),
            new Reg("TIMER1_RELOAD_N_REG", 0xFF_497A, DataTypes.u16),
            new Reg("TIMER2_DUTY1_REG", 0xFF_497C, DataTypes.u16),
            new Reg("TIMER2_DUTY2_REG", 0xFF_497E, DataTypes.u16),
            new Reg("TIMER2_FREQ_REG", 0xFF_4980, DataTypes.u16),
            new Reg("TIMER2_DUTY3_REG", 0xFF_4982, DataTypes.u16),
            new Reg("TONE_CTRL1_REG", 0xFF_4990, DataTypes.u16),
            new Reg("TONE_COUNTER1_REG", 0xFF_4992, DataTypes.u16),
            new Reg("TONE_LATCH1_REG", 0xFF_4994, DataTypes.u16),
            new Reg("TONE_CLEAR_INT1_REG", 0xFF_4996, DataTypes.u16),
            new Reg("TONE_CTRL2_REG", 0xFF_4998, DataTypes.u16),
            new Reg("TONE_COUNTER2_REG", 0xFF_499A, DataTypes.u16),
            new Reg("TONE_LATCH2_REG", 0xFF_499C, DataTypes.u16),
            new Reg("TONE_CLEAR_INT2_REG", 0xFF_499E, DataTypes.u16),
            new Reg("KEY_GP_INT_REG", 0xFF_49B0, DataTypes.u16),
            new Reg("KEY_BOARD_INT_REG", 0xFF_49B2, DataTypes.u16),
            new Reg("KEY_DEBOUNCE_REG", 0xFF_49B4, DataTypes.u16),
            new Reg("KEY_STATUS_REG", 0xFF_49B6, DataTypes.u16),
            new Reg("LCD_CTRL_REG", 0xFF_49C0, DataTypes.u16),
            new Reg("LCD_DAC_REG", 0xFF_49C2, DataTypes.u16),
            new Reg("ULP_CTRL_REG", 0xFF_4A00, DataTypes.u16),
            new Reg("ULP_INT_REG", 0xFF_4A02, DataTypes.u16),
            new Reg("ULP_PORT_REG", 0xFF_4A04, DataTypes.u16),
            new Reg("ULP_PHASE_REG", 0xFF_4A06, DataTypes.u16),
            new Reg("ULP_TIMERL_REG", 0xFF_4A08, DataTypes.u16),
            new Reg("ULP_TIMERH_REG", 0xFF_4A0A, DataTypes.u16),
            new Reg("ULP_WAKEUPL_REG", 0xFF_4A0C, DataTypes.u16),
            new Reg("ULP_WAKEUPH_REG", 0xFF_4A0E, DataTypes.u16),
            new Reg("ULP_STATUS_REG", 0xFF_4A10, DataTypes.u16),
            new Reg("ULP_TEST_REG", 0xFF_4A12, DataTypes.u16),
            new Reg("WATCHDOG_REG", 0xFF_4C00, DataTypes.u16),
            new Reg("SET_FREEZE_REG", 0xFF_5000, DataTypes.u16),
            new Reg("RESET_FREEZE_REG", 0xFF_5002, DataTypes.u16),
            new Reg("DEBUG_REG", 0xFF_5004, DataTypes.u16),
            new Reg("MEM_CONFIG_REG", 0xFF_5006, DataTypes.u16),
            new Reg("TRACE_CTRL_REG", 0xFF_5020, DataTypes.u16),
            new Reg("TRACE_STATUS_REG", 0xFF_5022, DataTypes.u16),
            new Reg("TRACE_START0_REG", 0xFF_5024, DataTypes.u16),
            new Reg("TRACE_LEN0_REG", 0xFF_5026, DataTypes.u16),
            new Reg("TRACE_START1_REG", 0xFF_5028, DataTypes.u16),
            new Reg("TRACE_LEN1_REG", 0xFF_502A, DataTypes.u16),
            new Reg("TRACE_TIMERL_REG", 0xFF_502C, DataTypes.u16),
            new Reg("TRACE_TIMERH_REG", 0xFF_502E, DataTypes.u16),
            new Reg("SET_INT_PENDING_REG", 0xFF_5400, DataTypes.u16),
            new Reg("RESET_INT_PENDING_REG", 0xFF_5402, DataTypes.u16),
            new Reg("INT0_PRIORITY_REG", 0xFF_5404, DataTypes.u16),
            new Reg("INT1_PRIORITY_REG", 0xFF_5406, DataTypes.u16),
            new Reg("INT2_PRIORITY_REG", 0xFF_5408, DataTypes.u16),
            new Reg("INT3_PRIORITY_REG", 0xFF_540A, DataTypes.u16),
            new Reg("PC_START_REG", 0xFF_540C, DataTypes.u16),
            new Reg("CODEC_MIC_REG", 0xFF_5800, DataTypes.u16),
            new Reg("CODEC_LSR_REG", 0xFF_5802, DataTypes.u16),
            new Reg("CODEC_VREF_REG", 0xFF_5804, DataTypes.u16),
            new Reg("CODEC_TONE_REG", 0xFF_5806, DataTypes.u16),
            new Reg("CODEC_ADDA_REG", 0xFF_5808, DataTypes.u16),
            new Reg("CODEC_OFFSET1_REG", 0xFF_580A, DataTypes.u16),
            new Reg("CODEC_TEST_CTRL_REG", 0xFF_580C, DataTypes.u16),
            new Reg("CODEC_OFFSET2_REG", 0xFF_580E, DataTypes.u16),
            new Reg("CODEC_MIC2_REG", 0xFF_5810, DataTypes.u16),
            new Reg("CODEC_MIC2_OFFSET1_REG", 0xFF_5812, DataTypes.u16),
            new Reg("CODEC_MIC2_OFFSET2_REG", 0xFF_5814, DataTypes.u16),
            new Reg("CODEC_MIC_AUTO_REG", 0xFF_5816, DataTypes.u16),
            new Reg("CODEC_LSR_AUTO_REG", 0xFF_5818, DataTypes.u16),
            new Reg("CLASSD_CTRL_REG", 0xFF_5C00, DataTypes.u16),
            new Reg("CLASSD_CLEAR_INT_REG", 0xFF_5C02, DataTypes.u16),
            new Reg("CLASSD_BUZZER_REG", 0xFF_5C04, DataTypes.u16),
            new Reg("CLASSD_TEST_REG", 0xFF_5C06, DataTypes.u16),
            new Reg("CLASSD_NR_REG", 0xFF_5C08, DataTypes.u16),
            new Reg("CLASSD_MIC2_REG", 0xFF_5C0A, DataTypes.u16),
            new Reg("DIP_STACK_REG", 0xFF_6000, DataTypes.u16),
            new Reg("DIP_STACK_REG.STACK_REG", 0xFF_6000, DataTypes.u16),
            new Reg("DIP_PC_REG", 0xFF_6002, DataTypes.u16),
            new Reg("DIP_STATUS_REG", 0xFF_6004, DataTypes.u16),
            new Reg("DIP_CTRL_REG", 0xFF_6006, DataTypes.u16),
            new Reg("DIP_STATUS1_REG", 0xFF_6008, DataTypes.u16),
            new Reg("DIP_CTRL1_REG", 0xFF_600A, DataTypes.u16),
            new Reg("DIP_SLOT_NUMBER_REG", 0xFF_600C, DataTypes.u16),
            new Reg("DIP_CTRL2_REG", 0xFF_600E, DataTypes.u16),
            new Reg("DIP_MOD_SEL_REG", 0xFF_6012, DataTypes.u16),
            new Reg("DIP_MOD_VAL_REG", 0xFF_6014, DataTypes.u16),
            new Reg("DIP_DC01_REG", 0xFF_6016, DataTypes.u16),
            new Reg("DIP_DC23_REG", 0xFF_6018, DataTypes.u16),
            new Reg("DIP_DC34_REG", 0xFF_601A, DataTypes.u16),
            new Reg("BMC_CTRL_REG", 0xFF_6400, DataTypes.u16),
            new Reg("BMC_CTRL2_REG", 0xFF_6402, DataTypes.u16),
            new Reg("BMC_TX_SFIELDL_REG", 0xFF_6404, DataTypes.u16),
            new Reg("BMC_TX_SFIELDH_REG", 0xFF_6406, DataTypes.u16),
            new Reg("BMC_RX_SFIELDL_REG", 0xFF_6408, DataTypes.u16),
            new Reg("BMC_RX_SFIELDH_REG", 0xFF_640A, DataTypes.u16),
            new Reg("RF_BURST_MODE_CTRL_REG", 0xFF_7000, DataTypes.u16),
            new Reg("RF_ALW_EN_REG", 0xFF_7008, DataTypes.u16),
            new Reg("RF_PORT_RSSI_SI_REG", 0xFF_700A, DataTypes.u16),
            new Reg("RF_TX_SI_REG", 0xFF_700C, DataTypes.u16),
            new Reg("RF_RX_SI_REG", 0xFF_700E, DataTypes.u16),
            new Reg("RF_PORT1_DCF_REG", 0xFF_7010, DataTypes.u16),
            new Reg("RF_PORT2_DCF_REG", 0xFF_7012, DataTypes.u16),
            new Reg("RF_PA_DRIVER_STAGE_DCF_REG", 0xFF_7014, DataTypes.u16),
            new Reg("RF_PA_FINAL_STAGE_DCF_REG", 0xFF_7016, DataTypes.u16),
            new Reg("RF_PLLCLOSED_DCF_REG", 0xFF_7018, DataTypes.u16),
            new Reg("RF_SYNTH_DCF_REG", 0xFF_701A, DataTypes.u16),
            new Reg("RF_BIAS_DCF_REG", 0xFF_701C, DataTypes.u16),
            new Reg("RF_RSSIPH_DCF_REG", 0xFF_701E, DataTypes.u16),
            new Reg("RF_DEM_DCF_REG", 0xFF_7020, DataTypes.u16),
            new Reg("RF_ADC_DCF_REG", 0xFF_7022, DataTypes.u16),
            new Reg("RF_IF_DCF_REG", 0xFF_7024, DataTypes.u16),
            new Reg("RF_LNAMIX_DCF_REG", 0xFF_7026, DataTypes.u16),
            new Reg("RF_PA_DCF_REG", 0xFF_7028, DataTypes.u16),
            new Reg("RF_FAD_WINDOW_DCF_REG", 0xFF_702A, DataTypes.u16),
            new Reg("RF_RFCAL_CTRL_REG", 0xFF_7040, DataTypes.u16),
            new Reg("RF_DEM_CTRL_REG", 0xFF_7044, DataTypes.u16),
            new Reg("RF_PREAMBLE_REG", 0xFF_7046, DataTypes.u16),
            new Reg("RF_RSSI_REG", 0xFF_7048, DataTypes.u16),
            new Reg("RF_PORT_CTRL_REG", 0xFF_704A, DataTypes.u16),
            new Reg("RF_PAD_IO_REG", 0xFF_704C, DataTypes.u16),
            new Reg("RF_PLL_CTRL1_REG", 0xFF_7050, DataTypes.u16),
            new Reg("RF_PLL_CTRL2_REG", 0xFF_7052, DataTypes.u16),
            new Reg("RF_PLL_CTRL3_REG", 0xFF_7054, DataTypes.u16),
            new Reg("RF_PLL_CTRL4_REG", 0xFF_7056, DataTypes.u16),
            new Reg("RF_SLICER_REG", 0xFF_7058, DataTypes.u16),
            new Reg("RF_SLICER_RESULT_REG", 0xFF_705A, DataTypes.u16),
            new Reg("RF_GAUSS_GAIN_RESULT_REG", 0xFF_705C, DataTypes.u16),
            new Reg("RF_BURST_MODE_SHADOW1_REG", 0xFF_7070, DataTypes.u16),
            new Reg("RF_BURST_MODE_SHADOW2_REG", 0xFF_7072, DataTypes.u16),
            new Reg("RF_DCF_MONITOR_REG", 0xFF_7074, DataTypes.u16),
            new Reg("RF_SYNTH_CTRL1_REG", 0xFF_7080, DataTypes.u16),
            new Reg("RF_SYNTH_CTRL2_REG", 0xFF_7082, DataTypes.u16),
            new Reg("RF_AGC_REG", 0xFF_7084, DataTypes.u16),
            new Reg("RF_AGC12_TH_REG", 0xFF_7086, DataTypes.u16),
            new Reg("RF_AGC12_ALPHA_REG", 0xFF_7088, DataTypes.u16),
            new Reg("RF_POSITIONING_REG", 0xFF_708A, DataTypes.u16),
            new Reg("RF_DC_OFFSET_REG", 0xFF_708C, DataTypes.u16),
            new Reg("RF_DC_OFFSET34_REG", 0xFF_708E, DataTypes.u16),
            new Reg("RF_IQ_DC_OFFSET_REG", 0xFF_7090, DataTypes.u16),
            new Reg("RF_IF_CTRL_REG", 0xFF_7092, DataTypes.u16),
            new Reg("RF_REF_OSC_REG", 0xFF_7094, DataTypes.u16),
            new Reg("RF_ADC_CTRL_REG", 0xFF_7096, DataTypes.u16),
            new Reg("RF_RFIO_CTRL_REG", 0xFF_7098, DataTypes.u16),
            new Reg("RF_BIAS_CTRL_REG", 0xFF_709A, DataTypes.u16),
            new Reg("RF_DRIFT_TEST_REG", 0xFF_709C, DataTypes.u16),
            new Reg("RF_TEST_MODE_REG", 0xFF_709E, DataTypes.u16),
            new Reg("RF_LDO_TEST_REG", 0xFF_70A0, DataTypes.u16),
            new Reg("RF_PLL_CTRL5_REG", 0xFF_70AC, DataTypes.u16),
            new Reg("RF_PLL_CTRL6_REG", 0xFF_70AE, DataTypes.u16),
            new Reg("RF_BBADC_CTRL_REG", 0xFF_70B0, DataTypes.u16),
            new Reg("RF_PA_CTRL1_REG", 0xFF_70B2, DataTypes.u16),
            new Reg("RF_PA_CTRL2_REG", 0xFF_70B4, DataTypes.u16),
            new Reg("RF_IFCAL_RESULT_REG", 0xFF_70B6, DataTypes.u16),
            new Reg("RF_DC_OFFSET12_REG", 0xFF_70B8, DataTypes.u16),
            new Reg("RF_AGC_RESULT_REG", 0xFF_70BA, DataTypes.u16),
            new Reg("RF_GAUSS_GAIN_MSB_REG", 0xFF_70BC, DataTypes.u16),
            new Reg("RF_RXFE_CTRL_REG", 0xFF_70BE, DataTypes.u16),
            new Reg("RF_FAFC_CTRL_REG", 0xFF_70C0, DataTypes.u16),
            new Reg("RF_FAFC_RESULT_REG", 0xFF_70C2, DataTypes.u16),
            new Reg("RF_TEST_MODE2_REG", 0xFF_70C4, DataTypes.u16),
            new Reg("FLASH_CTRL_REG", 0xFF_7400, DataTypes.u16),
            new Reg("FLASH_PTNVH1_REG", 0xFF_7402, DataTypes.u16),
            new Reg("FLASH_PTPROG_REG", 0xFF_7404, DataTypes.u16),
            new Reg("FLASH_PTERASE_REG", 0xFF_7406, DataTypes.u16),
            new Reg("FLASH_PTME_REG", 0xFF_7408, DataTypes.u16),
            new Reg("CHIP_TEST1_REG", 0xFF_FBF4, DataTypes.u8),
            new Reg("CHIP_TEST2_REG", 0xFF_FBF5, DataTypes.u8),
            new Reg("CHIP_ID1_REG", 0xFF_FBF8, DataTypes.u8),
            new Reg("CHIP_ID2_REG", 0xFF_FBF9, DataTypes.u8),
            new Reg("CHIP_ID3_REG", 0xFF_FBFA, DataTypes.u8),
            new Reg("CHIP_MEM_SIZE_REG", 0xFF_FBFB, DataTypes.u8),
            new Reg("CHIP_REVISION_REG", 0xFF_FBFC, DataTypes.u8),
            new Reg("CHIP_CONFIG1_REG", 0xFF_FBFD, DataTypes.u8),
            new Reg("CHIP_CONFIG2_REG", 0xFF_FBFE, DataTypes.u8),
            new Reg("CHIP_CONFIG3_REG", 0xFF_FBFF, DataTypes.u8),
        };

        for (var reg : regsSC14481) {
            addHwData(program, reg, as);
        }
    }

    private static void addHwData(Program program, Reg r, AddressSpace as) throws CodeUnitInsertionException, InvalidInputException {
        var addr = as.getAddress(r.address);
        DataUtilities.createData(program, addr, r.type, -1, false, DataUtilities.ClearDataMode.CLEAR_ALL_UNDEFINED_CONFLICT_DATA);
        program.getSymbolTable().createLabel(addr, r.name, SourceType.IMPORTED).setPinned(true);
    }
}
