/*******************************************************************************
*                                                                              *
* FILE:                                                                        * 
* 		prelaunch.c                                                            *
*                                                                              *
* DESCRIPTION:                                                                 * 
* 		Non-flight-qualified partition of APPA. Contains application loop for  *
*       idle state.                                                            *
*                                                                              *
* CRITICALITY:                                                                 *
*       NFQ - Non-Flight Qualified                                             *
*                                                                              *
*******************************************************************************/

/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "led.h"
#include "usb.h"
#include "math.h"
#include "sensor.h"
#include "common.h"
#include "commands.h"
#include "ignition.h"
#include "buzzer.h"


/*------------------------------------------------------------------------------
 Global Variables                                                                
------------------------------------------------------------------------------*/
extern PRESET_DATA preset_data;
extern SENSOR_DATA sensor_data;
extern FLIGHT_COMP_STATE_TYPE flight_computer_state;

/*------------------------------------------------------------------------------
 Functions                                                               
------------------------------------------------------------------------------*/

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		pre_launch_loop                                                        *
*                                                                              *
* DESCRIPTION:                                                                 *
* 		Application loop for the idle state.                                   *
*                                                                              *
*******************************************************************************/
void pre_launch_loop
    (
    uint8_t firmware_code,
    FLASH_STATUS* flash_status,
    HFLASH_BUFFER* flash_handle,
    uint32_t* flash_address,
    uint8_t* gps_mesg_byte,
    SENSOR_STATUS* sensor_status
    )
{
/*------------------------------------------------------------------------------
 Local Variables                                                                  
------------------------------------------------------------------------------*/
USB_STATUS    usb_status = USB_OK;             /* Status of USB HAL           */

/*--------------------------------------------------------------------------
 Handle invalid configs
--------------------------------------------------------------------------*/
if( *flash_status == FLASH_PRESET_NOT_FOUND )
	{
	led_set_color( LED_YELLOW );
	buzzer_multi_beeps(500, 500, 3);
	}

/*--------------------------------------------------------------------------
 USB MODE 
--------------------------------------------------------------------------*/
flight_computer_state = FC_STATE_IDLE;
led_set_color( LED_GREEN );

buzzer_multi_beeps(50, 50, 2);

while ( flight_computer_state == FC_STATE_IDLE )
    {
    usb_status = prelaunch_terminal
        ( 
        firmware_code,
        flash_status,
        flash_handle,
        flash_address,
        gps_mesg_byte,
        sensor_status
        );

    if( usb_status == USB_FAIL )
        {
        error_fail_fast( ERROR_USB_UART_ERROR );
        }

    } /* while ( flight_computer_state == FC_STATE_IDLE )*/

    error_fail_fast( ERROR_INVALID_STATE_ERROR );

} /* pre_launch_loop */


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		prelaunch_terminal                                                     *
*                                                                              *
* DESCRIPTION:                                                                 *
* 		Poll for USB input and execute terminal commands.                      *
*                                                                              *
*******************************************************************************/
USB_STATUS prelaunch_terminal
    ( 
    uint8_t firmware_code,
    FLASH_STATUS* flash_status,
    HFLASH_BUFFER* flash_handle,
    uint32_t* flash_address,
    uint8_t* gps_mesg_byte,
    SENSOR_STATUS* sensor_status
    )
{
/*------------------------------------------------------------------------------
 Local Variables                                                                  
------------------------------------------------------------------------------*/
USB_STATUS usb_status = USB_OK; /* USB Status*/
uint8_t usb_rx_data = 0;        /* USB Incoming Data Buffer    */
uint8_t subcommand_code = 0;    /* USB Subcommand Buffer       */

/*------------------------------------------------------------------------------
 Terminal Command Handler                                                               
------------------------------------------------------------------------------*/
led_set_color( LED_GREEN );

if ( usb_detect() )
    {
    /* Poll usb port */
    usb_status = usb_receive( &usb_rx_data, 
                            sizeof( usb_rx_data ), 
                            HAL_DEFAULT_TIMEOUT );

    /* Parse input code */
    if ( usb_status == USB_OK )
        {
        switch ( usb_rx_data )
            {
            /*-------------------------------------------------------------
                CONNECT_OP	
            -------------------------------------------------------------*/
            case CONNECT_OP:
                {
                /* Send board identifying code    */
                ping();

                /* Send firmware identifying code */
                usb_transmit( &firmware_code   , 
                            sizeof( uint8_t ), 
                            HAL_DEFAULT_TIMEOUT );
                break;
                } /* CONNECT_OP */

            /*--------------------------------------------------------------
                SENSOR Command	
            --------------------------------------------------------------*/
            case SENSOR_OP:
                {
                USB_STATUS command_status; /* Status of USB HAL           */
                /* Receive sensor subcommand  */
                command_status = usb_receive( &subcommand_code         ,
                                            sizeof( subcommand_code ),
                                            HAL_DEFAULT_TIMEOUT );

                if ( command_status == USB_OK )
                    {
                    /* Execute sensor subcommand */
                    sensor_cmd_execute( subcommand_code );
                    }
                else
                    {
                    error_fail_fast( ERROR_SENSOR_CMD_ERROR );
                    }
                break;
                } /* SENSOR_OP */
            
            /*-------------------------------------------------------------
                FIN_OP
            -------------------------------------------------------------*/
            case FIN_OP:
                {
                usb_status = finCalibration( &usb_rx_data );

                if ( usb_status != USB_OK )
                    {
                    error_fail_fast( ERROR_SERVO_CMD_ERROR );
                    }
                
                if ( write_preset(flash_handle, &preset_data, flash_address) != FLASH_OK )
                    {
                    error_fail_fast( ERROR_FLASH_CMD_ERROR );
                    }
                
                break;
                }

            /*-------------------------------------------------------------
                FLASH_OP 
            -------------------------------------------------------------*/
            case FLASH_OP:
                {
                /* Recieve flash subcommand over USB */
                usb_status = usb_receive( &subcommand_code       ,
                                        sizeof( subcommand_code ),
                                        HAL_DEFAULT_TIMEOUT );

                /* Execute subcommand */
                if ( usb_status == USB_OK )
                    {

                    /* Execute the subcommand */
                    *flash_status = flash_cmd_execute( subcommand_code,
                                                    flash_handle );
                    }
                else
                    {
                    /* Subcommand code not recieved */
                    error_fail_fast( ERROR_FLASH_CMD_ERROR );
                    }

                /* Transmit status code to PC */
                usb_status = usb_transmit( flash_status       ,
                                        sizeof( flash_status ),
                                        HAL_DEFAULT_TIMEOUT );

                if ( usb_status != USB_OK )
                    {
                    /* Status not transmitted properly */
                    error_fail_fast( ERROR_FLASH_CMD_ERROR );
                    }

                break;
                } /* FLASH_OP */
            /*-------------------------------------------------------------
                PRESET_OP  
            -------------------------------------------------------------*/
            case PRESET_OP:
                {
                /* Recieve preset subcommand over USB */
                usb_status = usb_receive( &subcommand_code       ,
                                        sizeof( subcommand_code ),
                                        HAL_DEFAULT_TIMEOUT );

                /* Execute subcommand */
                if ( usb_status == USB_OK )
                    {
                    /* Execute the subcommand */
                    *flash_status = preset_cmd_execute( &subcommand_code,
                                                        flash_handle,
                                                        flash_address  );
                    }
                else
                    {
                    /* Subcommand code not recieved */
                    error_fail_fast( ERROR_FLASH_CMD_ERROR );
                    }

                /* Transmit status code to PC */
                usb_status = usb_transmit( flash_status       ,
                                        sizeof( flash_status ),
                                        HAL_DEFAULT_TIMEOUT );

                if ( usb_status != USB_OK )
                    {
                    /* Status not transmitted properly */
                    error_fail_fast( ERROR_FLASH_CMD_ERROR );
                    }

                break;
                } /* PRESET_OP */
            /*-------------------------------------------------------------
                Unrecognized command code  
            -------------------------------------------------------------*/
            default:
                {
                // TODO: Give warning ( via error_fail_safe() )
                //error_fail_fast();
                break;
                }

            } /* switch( usb_rx_data ) */
        } /* if ( usb_status != USB_OK ) */
    } /* if ( usb_detect() )*/

/*------------------------------------------------------------------------------
 Arm Flight Computer                                                         
------------------------------------------------------------------------------*/
if ( ign_switch_cont() ) /* Enter flight mode */
    {
    if ( !check_config_validity( &preset_data ) )
        {
            error_fail_fast( ERROR_CONFIG_VALIDITY_ERROR );
        }
    
    /* check chute continuity */
    if( preset_data.config_settings.enabled_features & DUAL_DEPLOY_ENABLED )
        {
        if ( !ign_drogue_cont()
            || !ign_main_cont() )
            {
            buzzer_beep(3000);
            error_fail_fast( ERROR_IGNITION_CONTINUITY_ERROR );
            }
        }
    flight_loop( gps_mesg_byte, flash_status, flash_handle, flash_address, sensor_status);
    } /* if ( ign_switch_cont() )*/

return usb_status;

} /* prelaunch_terminal */

/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		preset_cmd_execute                                                     *
*                                                                              *
* DESCRIPTION:                                                                 *
* 		Execute a preset command.                                              *
*                                                                              *
*******************************************************************************/
FLASH_STATUS preset_cmd_execute
    ( 
    uint8_t* subcommand_code,
    HFLASH_BUFFER* flash_handle,
    uint32_t* flash_address
    )
{
enum SUBCMD_CODES {
    PRESET_UPLOAD = 0x01,
    PRESET_DOWNLOAD = 0x02,
    PRESET_VERIFY = 0x03
};
USB_STATUS usb_status = USB_OK;

switch (*subcommand_code)
    {

    /*-------------------------------------------------------------
     Upload Preset (to FC)
    -------------------------------------------------------------*/
    case PRESET_UPLOAD:
        {
        /* Recieve preset subcommand over USB */
        uint8_t data_receive_buffer[sizeof( CONFIG_SETTINGS_TYPE ) + 4];
        usb_status = usb_receive( data_receive_buffer,
                                sizeof( CONFIG_SETTINGS_TYPE ) + 4,
                                HAL_DEFAULT_TIMEOUT );

        /* Compute checksum */
        uint32_t checksum = crc32( &data_receive_buffer[4], sizeof( CONFIG_SETTINGS_TYPE ) );
        uint32_t received_checksum = 0;
        memcpy(&received_checksum, data_receive_buffer, 4);
        if (received_checksum == checksum)
            {
            /* data is valid! */
            memcpy((uint8_t*)(&preset_data) + 4, &(data_receive_buffer[4]), sizeof( CONFIG_SETTINGS_TYPE ) );
            }
        else {
            /* do not store checksum*/
            memcpy((uint8_t*)(&preset_data) + 4, &(data_receive_buffer[4]), sizeof( CONFIG_SETTINGS_TYPE ) );
            preset_data.checksum = 0;
            }

        return write_preset(flash_handle, &preset_data, flash_address);
        }

    /*-------------------------------------------------------------
     Download Preset (from FC)
    -------------------------------------------------------------*/
    case PRESET_DOWNLOAD:
        {
        FLASH_STATUS flash_status = FLASH_OK;
        uint8_t buffer[ sizeof(PRESET_DATA) ];
        // flash_status = write_preset( flash_handle, &preset_data, flash_address );
        memcpy(buffer, &preset_data, sizeof(PRESET_DATA));
        usb_status = usb_transmit( buffer, sizeof(PRESET_DATA), HAL_DEFAULT_TIMEOUT );
        
        if ( usb_status != USB_OK )
            {
            return FLASH_FAIL;
            }

        return flash_status;
        }

    /*-------------------------------------------------------------
     Verify Preset
    -------------------------------------------------------------*/
    case PRESET_VERIFY:
        {
        uint32_t checksum = crc32
            (
            (uint8_t*)(&(preset_data) + 4), /* pointer arithmetic; modify carefully */
            sizeof( PRESET_DATA ) - 4
            );
        uint8_t result = (checksum == preset_data.checksum);
        usb_status = usb_transmit( &result, 1, HAL_DEFAULT_TIMEOUT );
        

        if (usb_status != USB_OK)
            {
            error_fail_fast( ERROR_USB_UART_ERROR );
            }

        return FLASH_OK;
        }
    /*-------------------------------------------------------------
     Unrecognized command code  
    -------------------------------------------------------------*/
    default:
        {
        error_fail_fast( ERROR_USB_UART_ERROR );
        return FLASH_FAIL;
        }
    }

}


/*******************************************************************************
*                                                                              *
* PROCEDURE:                                                                   *
* 		check_config_validity                                                  *
*                                                                              *
* DESCRIPTION:                                                                 *
* 		Ensure no incompatibilities exist in configs.                          *
*                                                                              *
*******************************************************************************/
bool check_config_validity
    ( 
    PRESET_DATA* preset_data_ptr 
    )
{
/*-------------------------------------------------------------
 Local Variables 
-------------------------------------------------------------*/
bool valid = true;

/*-------------------------------------------------------------
 Postponed or deprecated feature check
-------------------------------------------------------------*/
if ( preset_data_ptr->config_settings.enabled_features &
     ( ACTIVE_PITCH_YAW_CONTROL_ENABLED 
     | WIRELESS_TRANSMISSION_ENABLED
     | ACTIVE_ROLL_CONTROL_ENABLED /* temporarily deprecated */ ) ) /* list invalid feature flags here*/
    {
    valid = false;
    }

/*-------------------------------------------------------------
 TODO: Validate checksum before proceeding
-------------------------------------------------------------*/

/*-------------------------------------------------------------
 TODO: Validate servo ranges before proceeding
-------------------------------------------------------------*/


/*-------------------------------------------------------------
 Handle invalid configs
-------------------------------------------------------------*/
while ( !valid )
    {
    led_set_color( LED_WHITE );
    buzzer_beep( 400 );
    led_set_color( LED_RED );
    HAL_Delay( 400 );
    }

return valid;

} /* check_config_validity */