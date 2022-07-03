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

class OMVRpcServiceQnapDashboard extends \OMV\Rpc\ServiceAbstract
{
    public function getName()
    {
        return "QnapControlDashboard";
    }

    public function initialize()
    {
        $this->registerMethod("getData");
    }

    public function getData($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $result = [];

        $result[] = [
            "prop" => "Service status",
            "priority" => 1,
            "data" => $this->isServiceActive()
        ];

        $result[] = [
            "prop" => "Fan",
            "priority" => 2,
            "data" => $this->getFanSpeed()
        ];

        $result[] = [
            "prop" => "Chasis temperature",
            "priority" => 3,
            "data" => $this->getChasisTemperature()
        ];

        $result = array_merge($result, $this->getCpuTemperatures());

        return $result;
    }

    function isServiceActive() {
        try {
            $cmd = new \OMV\System\Process("systemctl", "is-active", "panq-fan");
            $cmd->execute($output, $exitStatus);
            return $output;
        } catch (Exception $e) {
            return "inactive";
        }
    }

    function getFanSpeed() {
        if (file_exists("/usr/bin/panq")) {
            $cmd = new \OMV\System\Process("panq","fan");
            $cmd->execute($output, $exitStatus);

            if ($exitStatus == 0) {
                return $output;
            }
            return "Failed to get fan speed";
        } else {
            return "Panq binary not found";
        }
    }

    function getChasisTemperature() {
        if (file_exists("/usr/bin/panq")) {
            $cmd = new \OMV\System\Process("panq","temperature");
            $cmd->execute($output, $exitStatus);

            if ($exitStatus == 0) {
                return $output;
            }
            return "Failed to get fan speed";
        } else {
            return "Panq binary not found";
        }
    }

    function getCpuTemperatures() {
        $result = [];

        for ($i = 0 ; $i < 4 ; $i++) {
            $path = "/sys/devices/virtual/thermal/thermal_zone" . $i;

            $tempPath = $path . "/temp";
            $temp = file_get_contents($tempPath, false);
            if ($temp === false) {
                break;
            }

            $typePath = $path . "/type";
            $type = file_get_contents($typePath, false);

            if ($type === false) {
                $type = "Uknown sensor";
            }

            $result[] = [
                "prop" => $type,
                "priority" => $i + 100,
                "data" => (intval($temp) / 1000) . " °C"
            ];
        }

        return $result;
    }
}
