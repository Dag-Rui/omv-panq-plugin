<?php
/**
 * Copyright (C) 2022 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

class OMVRpcServiceQnapSettings extends \OMV\Rpc\ServiceAbstract
{

    public function getName()
    {
        return "QnapSettings";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getFanSpeed");
        $this->registerMethod("setFanSpeed");
    }

    public function getSettings($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $array = file('/etc/systemd/system/panq-fan.service');
        foreach($array as &$line) {
            if (substr($line, 0, 29) === "ExecStart=/usr/bin/panq start") {
                $tokens = explode(" ", $line);
                if (count($tokens) != 9) {
                    return null;
                }
                return [
                    "interval" => $tokens[2],
                    "minRpm" => $tokens[3],
                    "maxRpm" => $tokens[4],
                    "minCpuTemp" => $tokens[5],
                    "maxCpuTemp" => $tokens[6],
                    "minSystemTemp" => $tokens[7],
                    "maxSystemTemp" => str_replace("\r\n", "", $tokens[8])
                ];
            }
        }
    }

    public function setSettings($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        
        $execCommand = "ExecStart=/usr/bin/panq start " 
            .$params["interval"] . " "
            .$params["minRpm"] . " "
            .$params["maxRpm"] . " "
            .$params["minCpuTemp"] . " "
            .$params["maxCpuTemp"] . " "
            .$params["minSystemTemp"] . " "
            .$params["maxSystemTemp"] . "\r\n";
            

        $array = file('/etc/systemd/system/panq-fan.service');
        $data = "";
        foreach($array as &$line) {
            if (substr($line, 0, 29) === "ExecStart=/usr/bin/panq start") {
                $data .= $execCommand;
            } else {
                $data .= $line;
            }
        }

        $file = fopen('/etc/systemd/system/panq-fan.service', 'w');
        fwrite($file, $data);
        fclose($file);

        $cmd = new \OMV\System\Process("systemctl","daemon-reload");
        $cmd->execute($output, $exitStatus);

        $cmd = new \OMV\System\Process("systemctl","restart", "panq-fan");
        $cmd->execute($output, $exitStatus);
    }

    public function getFanSpeed($params, $context) {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        if (file_exists("/usr/bin/panq")) {
            $cmd = new \OMV\System\Process("panq","fan");
            $cmd->execute($output, $exitStatus);

            if ($exitStatus == 0) {
                return ["rpm" => $output];
            }
            
            return ["rpm" => "failed to get fan speed"];      
        } else {
            return ["rpm" => "panq binary not found"];   
        }
    }

    public function setFanSpeed($params, $context) {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        
        $percent = $params['rpm'];

        if (file_exists("/usr/bin/panq")) {
            $cmd = new \OMV\System\Process("panq","fan " . $percent);
            $cmd->execute($output, $exitStatus);

            if ($exitStatus == 0) {
                return "Ok";
            }
            return "Failed to set fan speed";
        } else {
            return "Panq binary not found";
        }
    }

}
