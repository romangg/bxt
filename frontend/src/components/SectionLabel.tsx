import {
    faCodeBranch,
    faCubes,
    faMicrochip
} from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import React from "react";

export type SectionLabelProps = React.HTMLProps<HTMLSpanElement> & {
    section?: ISection;
};

export default (props: SectionLabelProps) => {
    return (
        <span {...props}>
            <FontAwesomeIcon className="px-1" icon={faCodeBranch} />
            {props.section?.branch}
            <FontAwesomeIcon className="px-1" icon={faCubes} />
            {props.section?.repository}
            <FontAwesomeIcon className="px-1" icon={faMicrochip} />
            {props.section?.architecture}
        </span>
    );
};